# Network and Muserver interoperability handoff

## Purpose and bottom line

This page is the repository's handoff for the separate agent recreating
**Muserver**. It records what the client-recreation project has actually learned,
what remains unknown, and the safe prerequisites for making the two projects
interoperate. It is a summary and integration guide, **not** a decoded protocol
specification.

**There is no network client implementation in this repository today.** There
is no `src/mu/net` module, packet parser, socket wrapper, login/session flow,
Muserver integration, or authorized server test. Therefore there is currently
no recreated-client wire contract for Muserver to target, and compatibility
must not be claimed.

The safest path is to use an authorized protocol specification, or explicitly
agree on a new, versioned protocol for the independently recreated client and
server. The two projects must share that contract before either side treats
assumed packet formats as requirements.

## Evidence ledger

The labels below separate the existing repository audit from assumptions and
unexecuted work. The original client is read-only and its protected code is not
a source for protocol extraction.

| Topic | Status | What the evidence does and does not establish |
|---|---|---|
| Winsock imports in the reference executable | **CONFIRMED — import-table observation in the prior repository audit** | The audit records imports from `WS2_32`, including `WSASend`, `socket`, `connect`, `send`, and `recv`. Imports establish that socket APIs are present in the executable's import table; they do not prove which API is used for game traffic, which transport is used, or any packet layout. |
| HTTP-related imports / patch manifest | **PARTIAL** | The prior audit records HTTP-related imports and a `ListaPatch.txt` format consumed by the repository's asset verification tool. Patch/update checking is separate from the game-session protocol; it does not reveal login or gameplay messages. |
| `ProtocolCore.lua` and custom Lua files | **CONFIRMED — file-existence observation only** | The prior asset inventory records these files. Their existence does not establish the binary wire schema, opcode values, cryptography, or a complete login flow. |
| Handshake, login, and map loading | **PROBABLE, not a specification** | Earlier repository notes infer these broad stages from strings and scripts. No exact message sequence or server/client state machine has been validated. |
| MU-style `C1` / `C2` packet headers | **UNVERIFIED for this reference client** | These are common in MU-related material, but the existing evidence does not prove that this protected executable uses a particular framing or that a new client/server should copy it. Do not hard-code this assumption. |
| Transport, endpoints, framing, endianness, length limits, opcodes, payloads, compression, packet encryption/key exchange | **UNKNOWN** | No client-side network implementation or authorized protocol trace/specification is available in this project. Do not fill these fields by guesswork. |
| License / anti-cheat behavior | **BLOCKED / out of scope** | The reference stack includes protected and commercial components. Do not unpack, patch, emulate, bypass, or weaken licensing or anti-cheat. Their presence does not establish a game-protocol requirement. |
| Recreated-client network code and Muserver test | **NOT IMPLEMENTED / NOT EXECUTED** | No `mu/net` code was found under `src/mu`; no authorized Muserver endpoint or test session was used. |

The import-table and file observations above are carried forward from the
repository's read-only audit; they were not re-measured during the cycle that
created this handoff. See [`REFERENCE_AUDIT.md`](REFERENCE_AUDIT.md),
[`CURRENT_STATE.md`](CURRENT_STATE.md), and [`SECURITY.md`](SECURITY.md) for
scope, provenance, and limitations.

## What the Muserver agent should assume today

- This client project currently provides asset readers, diagnostic tools, and
  partial CPU-side rendering foundations—not a playable client or networking
  implementation.
- There are no authoritative client packet IDs, login fields, server-list
  messages, character messages, world-state messages, encryption keys, or
  timeout values in this repository.
- `ListaPatch.txt` verification is an asset/update-manifest concern. It must not
  be treated as a game login or gameplay protocol.
- Asset compatibility and server compatibility are separate acceptance areas.
  A server can be tested against a documented protocol before the client can
  render a map; conversely, parsed assets do not prove that a server can talk to
  this client.
- The original-client feature names or the presence of socket imports are not
  evidence that the corresponding behavior has been implemented in the new
  client.

If the Muserver project already has an authorized specification, its agent can
record and implement that specification independently. It must still avoid
claiming compatibility with this recreated client until this client implements
the same version and an integration test passes.

## Prerequisite: one shared, versioned protocol contract

Before client and server code depend on one another, select one of these routes:

1. **Authorized existing specification:** record the document/source, version,
   scope, and permission to use it. Keep assumptions that are not covered by the
   source marked unknown.
2. **New protocol for this independent client/server pair:** agree explicitly
   with the project owner that the new protocol—not presumed compatibility with
   the protected original—is acceptable, then publish one canonical,
   versioned specification and link to the same revision from both projects.

Do not quietly choose opcode values, cryptography, `C1`/`C2` framing, or a
transport in this handoff. The protocol specification must settle, at minimum:

- transport, connection lifecycle, and any secure-channel requirements;
- frame structure, byte order, maximum frame size, and malformed-frame rules;
- protocol version negotiation, message identifiers, and payload schemas;
- account authentication flow using test credentials, error handling, throttling,
  timeouts, disconnects, and reconnect behavior;
- server/realm selection, character operations, and session state transitions;
- world/map identifiers, coordinate units, movement ownership, update cadence,
  and keepalive behavior;
- how server-side entity/item/skill identifiers map to the client's local data;
- compatibility policy for optional fields and future protocol versions.

The values and message formats are **not defined here**. Leaving them open is
intentional: this repository has no evidence from which to infer them safely.

## Suggested joint implementation and acceptance sequence

1. **Agree and publish the contract.** Identify the authorized source or approve
   a new clean protocol. Add a version and reviewable schema/examples; do not
   put real passwords, account data, proprietary assets, or protected-client
   captures in Git.
2. **Test each implementation against the same fixtures.** Use synthetic,
   non-secret messages generated from the agreed specification. Include positive
   cases and rejection cases for truncated frames, invalid lengths, unknown
   messages, and invalid state transitions. Keep the client and server tests
   aligned to the same schema version.
3. **Build an isolated loopback integration test.** Use a local authorized test
   server, disposable test accounts, deterministic data, and explicit expected
   state transitions. Record exact client/server revisions and logs with
   credentials redacted.
4. **Add gameplay stages incrementally.** Do not report full compatibility from
   a socket connection or successful login alone. Test server selection,
   character/session state, map entry, movement, inventory, combat, and
   persistence only as each feature is implemented in both projects and has a
   documented acceptance test.
5. **Keep server authority and security checks intact.** Validate all input and
   state transitions server-side. Do not weaken authentication, permissions,
   rate limits, licensing, or anti-cheat to make a test pass. Never log secrets.
6. **Record failures as evidence, not assumptions.** Store reproducible
   synthetic test output in each project's test/evidence area. A failed test
   does not authorize inferring or bypassing protected behavior.

Until the shared specification and loopback test exist, the server-side status
for this client is **NOT EXECUTED**, not compatible or incompatible.

## Handoff reading map

This page is the entry point for the Muserver agent, not a replacement for the
client project history. Read these sources for the full process and current
limits:

- [`AGENTS.md`](../AGENTS.md) — permanent scope, preservation, licensing,
  security, test, and branch rules.
- [`CURRENT_STATE.md`](CURRENT_STATE.md) and
  [`COMPATIBILITY.md`](COMPATIBILITY.md) — what is implemented, partial,
  blocked, and not executed.
- [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`NEXT_TASK.md`](NEXT_TASK.md) —
  intended modules and current implementation order.
- [`REFERENCE_AUDIT.md`](REFERENCE_AUDIT.md) — read-only reference findings and
  their provenance.
- [`SECURITY.md`](SECURITY.md), [`LICENSING.md`](LICENSING.md), and
  [`TEST_RESULTS.md`](TEST_RESULTS.md) — security boundaries and actual test
  evidence.
- [`ASSET_FORMATS.md`](ASSET_FORMATS.md) — asset-format work only; it is not a
  network-protocol document.
- [`CHANGELOG.md`](CHANGELOG.md) and [`evidence/`](evidence/) — cycle history
  and raw evidence. The latest Linux validation log records asset-library
  tests only; it is not a client/server test.

## Current state of this handoff

This documentation records and clarifies existing evidence; it adds no socket,
packet, authentication, or server code and does not make any compatibility
claim. The next networking action is to obtain an authorized protocol source or
an explicit decision to define a new shared contract. After that, the client
project still needs its own `mu/net` implementation and authorized loopback
integration tests.
