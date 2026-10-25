# tinycdbff House Rules

The shared house rules come from skilletor — bundle `getty-house@getty`: engineering
discipline, the delegation lock, the publish gate, karr coordination. They load as
`.claude/rules/.local.*.md`; if they are missing, run `skilletor sync` and publish nothing
until they load. This file adds what is true only here, and wins where it differs.

## What this repo is

A public-domain port of Michael Tokarev's TinyCDB (a constant database, based on djb's cdb)
to Chan FatFs — a source-drop module that consumers vendor and link against their own FatFs.
Read first: `cdb.h`, `cdb_int.h` and the caller of the function you touch.

## Agent team

| Task | Agent |
|---|---|
| Implement / refactor / debug behaviour-relevant code | `tinycdbff-worker` (default) |
| Write / extend tests | `tinycdbff-test-writer` |
| Commits, card → done | `tinycdbff-release-manager` |
| Read-only research, audit, gap analysis | `tinycdbff-worker` ("read-only, change nothing") |

Behaviour-relevant = the C sources: runtime behaviour, the public API in `cdb.h`, the on-disk
format, error handling and the test harness. Pure prose (README, CLAUDE.md, this file) is not.

## Release

There is **no package release** — tinycdbff is vendored source, nothing is built or published.
`sh t/compile-check.sh` and the behavioural tests are fine anytime; `git push`, tags and the
GitHub tracker (`cindustries/tinycdbff`) sit under the publish gate.

## Hazards

- **A faithful port of upstream tinycdb.** A deviation from upstream must be deliberate and
  explained. Keep the C style, the deliberate `cdb_pack` shift form and the terse variable
  names.
- **compile-check is syntax-only.** `t/compile-check.sh` passing means the sources *compile*
  against an opaque stub, not that the index works. Never claim behaviour is verified on the
  compile-check alone — behaviour needs the in-memory FatFs shim.
- **FRESULT is never negative.** `f_lseek`/`f_read`/`f_write` return an enum (`FR_OK == 0`);
  `if (f_lseek(...) < 0)` is dead error handling — check `!= FR_OK`. (Detail: skill `tinycdbff-core`.)
- **The on-disk layout is a compatibility contract** — 2048-byte TOC, little-endian
  4-byte ints, djb hash. Changing the format silently breaks every reader.
- **Embedded target, no POSIX.** Don't introduce hosted-only headers (`<unistd.h>`
  etc.) or libc beyond `string.h`/`stdlib.h`/`errno.h`.
- **Public-domain header** stays on every source file; no incompatibly-licensed code.

## C specifics — reference, don't restate

The format contract, the FatFs seam, the API conventions, the portability rule and the test model
live in skill `tinycdbff-core` (force-loaded per lane via `briefing.skills`).
