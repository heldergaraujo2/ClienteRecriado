# Security

Risks found in the reference client and in this project's own practice. Secret
values are **not** reproduced anywhere in this repository — only the kind of
risk and its location.

## 1. Credential exposure in the reference client — HIGH

**Finding.** `Settings.ini` in the reference client contains an `[AutoLogin]`
section with a plaintext username, a plaintext password, and a `SaveData` flag.
The file is committed to the repository.

**Why this matters.** The credentials are in the Git history, so deleting the
file would not remove them. Anyone with read access to the repository has them.
If the password is reused on any other service, the exposure extends there.

**Recommended action, for the owner of that repository.**

1. Rotate the password immediately, and change it anywhere else it is used.
2. Remove the credential values from `Settings.ini` and ship it with empty
   fields.
3. Add `Settings.ini` to `.gitignore`, or split user credentials into a separate
   file that is never committed.
4. Note that history rewriting would be needed to fully remove the values, and
   that this may break existing clones. Rotation is the action that actually
   closes the risk; history rewriting is cosmetic in comparison.

**Action taken here.** None of these values were copied into this repository,
into a commit message, or into a log. This document refers to the keys only. A
check was added to the review checklist in `AGENTS.md`.

## 2. Protection components — do not touch

The client includes commercial licensing and anti-cheat modules: `License.hinet`,
`main.premium`, `3D.Premium`, `Checksum.premium`, `MHPClient.dll`,
`MHPVerify.dll`, `3DGuard.dll`, and `PS/License.ps`. `Main.exe` is packed with
VMProtect.

`ListaPatch.txt` carries checksums for `main.premium`, `3D.Premium` and
`License.hinet`, so the client integrity-checks its own protection files.

**Policy.** No unpacking, patching, key generation, licence check removal or
circumvention of any kind. This is a hard rule, restated in `AGENTS.md`. Tasks
that appear to require it are out of scope and get recorded as `BLOCKED`.

This is not a technical limitation being dressed up as a policy. Even setting
licensing aside, modifying a protected module would produce something that
cannot be distributed and cannot be maintained.

## 3. Untrusted input handling

Every reader in `src/mu/assets/` processes files that may be corrupt, truncated
or deliberately hostile — a patch server compromise or a malformed download both
land here. The rules the code follows:

- **No reader indexes a buffer without a length check.** Every access is guarded.
- **No reader trusts a length field read from the file.** Declared sizes are
  compared against the bytes actually present, and the smaller is used. A
  truncated download degrades instead of overflowing.
- **Multiplication is guarded against overflow.** OZT dimensions are checked
  against a cap before `width * height * bytes_per_pixel` is computed, so a
  corrupt header cannot wrap to a small value and pass the size check.
- **Readers return `mu::Status`, never throw, never abort.** A single bad file
  in a 29,000-file tree must not take down a scan.
- **Parsing continues past a bad record.** `ManifestParse` records a line number
  and moves on, because one corrupt line should not hide the other 187 entries.

Verified by: truncation tests at multiple offsets for every reader,
out-of-range dimension tests, absurd length tests, and a full run under
AddressSanitizer and UndefinedBehaviorSanitizer with no reports.

## 4. Supply chain

The library has **no third-party dependencies**. There is no `vcpkg.json`, no
vendored tarball, no download step and no package manager in the build. The
attack surface for a dependency compromise is zero, and there are no transitive
licences to track. See [`DEPENDENCIES.md`](DEPENDENCIES.md).

The trade-off is that there is no JPEG pixel decoder. That gap is recorded
honestly rather than closed with an unaudited vendored copy of something.

## 5. Secrets in this repository

None. Checked:

- No credential, token, API key or private key material in any file.
- No `Settings.ini` values copied from the reference client.
- No original asset bytes.
- `.gitignore` excludes build output.

The reference client's `mu.ini` is 111 bytes of binary content of unknown
purpose. It was not copied here and its contents are not reproduced.

## 6. Distribution integrity

`ListaPatch.txt` provides `path crc32 size` records, and `mutool verify`
implements the same CRC-32 (verified against the reference client's own recorded
value for `Data/Item/capesnowlucasnuut02.ozt`). That makes it possible to
integrity-check a distribution before shipping and after install.

**Note on CRC-32 as a security control.** CRC-32 detects accidental corruption
well. It is **not** collision resistant and must not be used to detect
deliberate tampering. A distribution pipeline that needs tamper evidence should
use SHA-256 alongside it. This is recorded so nobody later mistakes the existing
CRC manifest for a security mechanism.

**Current state of the reference client's own manifest.** Verification against
the tree returned 116 matching of 188 entries, with 9 size mismatches, 11 CRC
mismatches and 52 missing files. The shipped client does not match its own
integrity manifest. Details in [`REFERENCE_AUDIT.md`](REFERENCE_AUDIT.md)
section 6.

## 7. Privacy

Twelve `Thumbs.db` files (677,376 bytes) are present in the asset tree. These
are Windows Explorer thumbnail caches and they embed a record of the directory
listing on the machine that packaged the client. They serve no purpose in a
distribution and leak information about whoever built it. `mutool scan` counts
them separately so a packaging step can exclude them.

The `[Discord] DiscordShowPresence = 1` setting means the client reports
presence to a third-party service by default. Anyone redistributing should be
aware that this is on unless the user turns it off.

## 8. Server-side validation

Not weakened, not bypassed, not worked around, and no benchmark in this project
depends on doing so. There is no authorised test server available, so network
compatibility is recorded as `NOT EXECUTED` rather than approximated.
