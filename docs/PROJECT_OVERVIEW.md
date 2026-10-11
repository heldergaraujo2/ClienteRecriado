# Project overview

## 1. Goal

Reconstruct and progressively optimise a MU Online client, prioritising frame
rate, fluidity, low interaction latency, fast loading, stability, and visual and
functional compatibility with the original.

## 2. The constraint that shapes everything

The audit established one fact that determines the entire strategy: **the
original engine cannot be modified.**

`Main.exe` in the reference client is packed with VMProtect — sections `.vmp0`
and `.vmp1`, with `.text`, `.rdata` and `.data` having zero size on disk. Its
import table shows 51 `OPENGL32` functions including `wglCreateContext`,
`glBegin`/`glEnd` and `SwapBuffers`, which places the render loop inside the
protected module. Around it sits a commercial licensing and anti-cheat stack:
`License.hinet`, `main.premium`, `3D.Premium`, `Checksum.premium`,
`MHPClient.dll`, `MHPVerify.dll`, `3DGuard.dll`.

Bypassing any of that is out of scope and will not be attempted. So the
available routes are:

1. **Read the assets.** No asset format is protected. This is the route to
   visual parity, and it is where most of the work so far has gone.
2. **Build an independent renderer** that consumes those assets. This is the
   route to better performance, because the original renderer's structure is
   known from its import tables and can be improved upon in new code.
3. **Optimise externally** — configuration, asset hygiene, packaging, load
   order — without touching protected code.

## 3. Scope

### In scope

- Independent readers for every asset format in the client, with tests against
  the real asset tree.
- Diagnostic and measurement tooling that produces real numbers rather than
  estimates.
- An independent renderer and engine, built incrementally, that loads the
  original assets unmodified.
- A reproducible build and packaging process for Windows.
- Automated tests, benchmarks and regression detection.

### Out of scope

- Unpacking, patching, keygenning or defeating VMProtect or any licence or
  anti-cheat check.
- Reverse engineering the protected module.
- Redistributing original game assets, binaries or any third-party component
  whose redistribution rights are unconfirmed.
- Modifying or weakening server-side validation to make benchmarks look better.

## 4. Performance priorities

In the order given, which is also the order work should be taken:

1. Eliminate crashes, memory corruption, deadlocks and freezes.
2. Remove bottlenecks that cap frame rate and responsiveness.
3. Reduce stutter, frametime spikes and loading pauses.
4. Reduce input latency and improve interface responsiveness.
5. Reduce startup and map/model/texture load time.
6. Improve CPU, GPU, RAM and VRAM usage.
7. Preserve the fidelity of the original 3D assets.
8. Improve stability in long sessions and crowded scenes.
9. Preserve network, protocol and content compatibility.
10. Automate tests, measurements and regression detection.

Note that frame rate is not the only measure of fluidity. Frametime consistency,
spike frequency, input latency and resource usage all matter, and a change that
raises mean FPS while worsening p99 frametime is a regression.

## 5. Current state in one paragraph

The asset layer works. 99.31% of OZJ textures, 98.10% of OZT textures, 99.76%
of BMD containers and 100% of TGA and terrain files in a real 2,372 MB asset
tree are parsed and classified by code in this repository, backed by 203 unit
test checks. The reference client's renderer is characterised from its import
tables: fixed-function OpenGL 1.1, immediate mode plus client-side vertex arrays
plus display lists, no VBOs, no FBOs, `glReadPixels` on the presentation path.
The measured texture memory requirement of the asset set is 2,893.80 MB
uncompressed. What does not exist yet is a renderer, and the BMD model payload
cipher is not recovered, which blocks geometry and animation.

Full detail: [`CURRENT_STATE.md`](CURRENT_STATE.md).

## 6. What "done" means

The project is complete only when evidence demonstrates:

- An independent, documented architecture with implemented and pending
  components clearly separated.
- A reproducible build.
- Required assets loading correctly.
- Declared functionality actually implemented — not stubbed, not simulated.
- A documented performance baseline with reproducible benchmark scenarios.
- Optimisations justified by evidence, compared before and after under
  equivalent conditions.
- FPS, frametime and resource metrics recorded.
- No critical regressions in the tests that could be run.
- Automated tests implemented, with results correctly classified.
- Dependencies and licences documented.
- No secrets in the package.
- Build, run and validation instructions available.
- The next task recorded while work remains.

**No absolute performance claim will be made.** Results will always be stated
with the hardware, the conditions, the known limitations and what remains
uninvestigated.
