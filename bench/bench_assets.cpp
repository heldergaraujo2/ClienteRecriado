// SPDX-License-Identifier: MIT
// Micro-benchmarks for the asset library.
//
// This measures CPU-side decode work only. It is NOT a frame rate measurement
// and must never be quoted as one: there is no GPU, no window and no driver
// here. What it does establish is the cost of the parsing and pixel conversion
// step, which is the part of texture loading that a reconstruction controls
// directly.
//
// Methodology:
//   - Each benchmark runs a warm-up, then N timed repetitions.
//   - The clock is steady_clock, read around the smallest unit that still
//     measures above timer noise.
//   - Median and the 95th/99th percentiles are reported, not just the mean.
//     For load-time work the tail is what the player feels: a mean of 2 ms with
//     a p99 of 40 ms is a hitch, not a fast loader.
//   - Results are printed as a table so two runs can be diffed directly.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mu/assets/bmd.hpp"
#include "mu/assets/format.hpp"
#include "mu/assets/manifest.hpp"
#include "mu/assets/ozj.hpp"
#include "mu/assets/ozt.hpp"
#include "mu/assets/tga.hpp"

namespace {

using Clock = std::chrono::steady_clock;

struct Samples {
  std::vector<double> us;  // microseconds
  double median = 0, p95 = 0, p99 = 0, mean = 0, total = 0;
  double stddev = 0;       ///< Population standard deviation of the samples.
  double min = 0, max = 0;
};

Samples Summarise(std::vector<double> v) {
  Samples s;
  s.us = std::move(v);
  if (s.us.empty()) return s;
  std::sort(s.us.begin(), s.us.end());
  const std::size_t n = s.us.size();
  auto pct = [&](double p) {
    std::size_t i = static_cast<std::size_t>(p * static_cast<double>(n - 1));
    return s.us[i];
  };
  s.median = pct(0.50);
  s.p95 = pct(0.95);
  s.p99 = pct(0.99);
  s.min = s.us.front();
  s.max = s.us.back();
  double sum = 0;
  for (double x : s.us) sum += x;
  s.mean = sum / static_cast<double>(n);
  double acc = 0;
  for (double x : s.us) {
    const double d = x - s.mean;
    acc += d * d;
  }
  s.stddev = std::sqrt(acc / static_cast<double>(n));
  s.total = sum;
  return s;
}

template <typename F>
Samples TimeIt(int reps, F&& fn) {
  fn();  // warm-up: first touch pays for page faults and cache misses
  std::vector<double> v;
  v.reserve(static_cast<std::size_t>(reps));
  for (int i = 0; i < reps; ++i) {
    const auto t0 = Clock::now();
    fn();
    const auto t1 = Clock::now();
    v.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
  }
  return Summarise(std::move(v));
}

void PrintRow(const char* name, std::size_t bytes, const Samples& s) {
  const double mb = static_cast<double>(bytes) / 1048576.0;
  // Throughput in MB/s from the median, which is the stable statistic.
  const double mbps = s.median > 0 ? mb / (s.median / 1e6) : 0.0;
  std::printf("%-34s %8.1f %8.1f %8.1f %8.1f %10.2f %9.1f\n", name, s.median,
              s.p95, s.p99, s.mean, mb, mbps);
}

std::vector<uint8_t> MakeRgba(std::size_t w, std::size_t h) {
  std::vector<uint8_t> p(w * h * 4);
  for (std::size_t i = 0; i < p.size(); ++i)
    p[i] = static_cast<uint8_t>(i * 31 + 7);
  return p;
}

}  // namespace

int main() {
  std::printf("ClienteRecriado asset micro-benchmarks\n");
  std::printf("CPU-side decode cost only. Not a frame rate measurement.\n\n");
  std::printf("%-34s %8s %8s %8s %8s %10s %9s\n", "benchmark", "med us",
              "p95 us", "p99 us", "mean us", "MB", "MB/s");
  std::printf("%-34s %8s %8s %8s %8s %10s %9s\n", "---------", "------",
              "------", "------", "-------", "--", "----");

  // ------------------------------------------------ OZT probe (header only)
  {
    const std::size_t w = 256, h = 256;
    std::vector<uint8_t> rgba = MakeRgba(w, h);
    std::vector<uint8_t> blob;
    mu::OztWriteRgba(static_cast<uint32_t>(w), static_cast<uint32_t>(h),
                     rgba.data(), true, &blob);
    mu::OztImage img;
    Samples s = TimeIt(20000, [&] {
      mu::OztProbe(mu::ByteView(blob.data(), blob.size()), &img);
    });
    PrintRow("OztProbe 256x256", blob.size(), s);

    // Full decode: header validation plus the pixel copy.
    mu::OztImage full;
    s = TimeIt(2000, [&] {
      mu::OztRead(mu::ByteView(blob.data(), blob.size()), &full);
    });
    PrintRow("OztRead 256x256 (copy pixels)", blob.size(), s);

    // Conversion to the layout glTexImage2D wants.
    std::vector<uint8_t> out;
    s = TimeIt(2000, [&] { mu::OztToRgbaTopDown(full, &out); });
    PrintRow("OztToRgbaTopDown 256x256", full.pixel_bytes(), s);
  }

  // ------------------------------------------------ OZT at larger sizes
  for (std::size_t dim : {512u, 1024u}) {
    std::vector<uint8_t> rgba = MakeRgba(dim, dim);
    std::vector<uint8_t> blob;
    mu::OztWriteRgba(static_cast<uint32_t>(dim), static_cast<uint32_t>(dim),
                     rgba.data(), true, &blob);
    mu::OztImage full;
    Samples s = TimeIt(400, [&] {
      mu::OztRead(mu::ByteView(blob.data(), blob.size()), &full);
    });
    char label[64];
    std::snprintf(label, sizeof(label), "OztRead %zux%zu", dim, dim);
    PrintRow(label, blob.size(), s);
  }

  // ------------------------------------------------ bux cipher throughput
  {
    std::vector<uint8_t> buf(1 << 20, 0x5A);
    Samples s = TimeIt(200, [&] { mu::BuxCrypt(buf.data(), buf.size()); });
    PrintRow("BuxCrypt 1 MiB", buf.size(), s);
  }

  // ------------------------------------------------ entropy measurement
  {
    std::vector<uint8_t> buf(1 << 20);
    uint32_t x = 7;
    for (std::size_t i = 0; i < buf.size(); ++i) {
      x = x * 1103515245u + 12345u;
      buf[i] = static_cast<uint8_t>((x >> 16) & 0xFF);
    }
    Samples s = TimeIt(200, [&] { mu::ByteEntropy(buf.data(), buf.size()); });
    PrintRow("ByteEntropy 1 MiB", buf.size(), s);
  }

  // ------------------------------------------------ CRC-32 throughput
  //
  // Reported in more detail than the other rows, because this number is used
  // to justify an optimisation and therefore needs its dispersion and its
  // cache state stated, not just a median.
  {
    for (std::size_t kib : {64u, 1024u, 16384u}) {
      const std::size_t bytes = kib * 1024;
      std::vector<uint8_t> buf(bytes, 0x3C);
      volatile uint32_t sink = 0;
      const int reps = bytes <= (1u << 20) ? 300 : 20;
      Samples s = TimeIt(reps, [&] { sink = mu::Crc32(buf.data(), buf.size()); });
      char label[64];
      std::snprintf(label, sizeof(label), "Crc32 %zu KiB (in cache)", kib);
      PrintRow(label, buf.size(), s);
      std::printf("    reps=%d  mean=%.1f us  stddev=%.1f us (%.2f%% of mean)"
                  "  min=%.1f  max=%.1f\n",
                  reps, s.mean, s.stddev,
                  s.mean > 0 ? 100.0 * s.stddev / s.mean : 0.0, s.min, s.max);
    }

    // The same work against a file read from disk on every repetition, so the
    // buffer is not resident. This is what `mutool verify` actually does, and
    // it is the honest comparison: an in-cache CRC number overstates what a
    // whole-tree verification achieves.
    const char* disk_path = std::getenv("MU_BENCH_FILE");
    if (disk_path && *disk_path) {
      std::vector<uint8_t> buf;
      volatile uint32_t sink = 0;
      Samples s = TimeIt(20, [&] {
        if (mu::ReadFile(disk_path, &buf)) sink = mu::Crc32(buf.data(), buf.size());
      });
      std::printf("%-34s %8.1f %8.1f %8.1f %8.1f %10.2f %9.1f\n",
                  "Crc32 + disk read each rep", s.median, s.p95, s.p99, s.mean,
                  static_cast<double>(buf.size()) / 1048576.0,
                  s.median > 0
                      ? static_cast<double>(buf.size()) / 1048576.0 / (s.median / 1e6)
                      : 0.0);
      std::printf("    reps=20  mean=%.1f us  stddev=%.1f us  file=%s\n", s.mean,
                  s.stddev, disk_path);
    } else {
      std::printf("Crc32 + disk read: NOT RUN (set MU_BENCH_FILE to a real file"
                  " to include it)\n");
    }
  }

  // ------------------------------------------------ manifest parsing
  {
    std::string text;
    text.reserve(64 << 10);
    for (int i = 0; i < 5000; ++i) {
      char line[128];
      std::snprintf(line, sizeof(line), "Data/Item/asset_%05d.ozj %08x %d\n", i,
                    static_cast<uint32_t>(i) * 2654435761u, 1000 + i);
      text += line;
    }
    mu::ManifestParseResult r;
    Samples s = TimeIt(200, [&] {
      r = mu::ManifestParse(
          mu::ByteView(reinterpret_cast<const uint8_t*>(text.data()),
                       text.size()));
    });
    PrintRow("ManifestParse 5000 entries", text.size(), s);
    std::printf("  (parsed %zu entries)\n", r.entries.size());
  }

  // ------------------------------------------------ JPEG marker walk
  {
    // A synthetic JPEG with a large entropy-coded segment, so the scan-skip
    // path is exercised rather than a trivially short file.
    std::vector<uint8_t> j;
    auto put = [&](uint8_t b) { j.push_back(b); };
    auto putbe = [&](uint16_t v) {
      j.push_back(static_cast<uint8_t>(v >> 8));
      j.push_back(static_cast<uint8_t>(v & 0xFF));
    };
    put(0xFF); put(0xD8);
    put(0xFF); put(0xE0); putbe(16);
    for (int i = 0; i < 14; ++i) put(0x00);
    put(0xFF); put(0xC0); putbe(17);
    put(8); putbe(512); putbe(512); put(3);
    put(1); put(0x22); put(0); put(2); put(0x11); put(1); put(3); put(0x11); put(1);
    put(0xFF); put(0xDA); putbe(8);
    put(1); put(1); put(0x00); put(0); put(63); put(0);
    for (int i = 0; i < 200000; ++i) put(static_cast<uint8_t>(i & 0x7F) | 0x01);
    put(0xFF); put(0xD9);

    mu::OzjImage img;
    Samples s = TimeIt(200, [&] {
      mu::OzjProbe(mu::ByteView(j.data(), j.size()), &img);
    });
    PrintRow("OzjProbe 512x512 + 200KB scan", j.size(), s);
    std::printf("  (geometry %ux%u)\n", img.width, img.height);
  }

  std::printf("\nRun twice and compare the medians; a shift of more than a few\n"
                "percent on the same machine means something changed.\n");
  return 0;
}
