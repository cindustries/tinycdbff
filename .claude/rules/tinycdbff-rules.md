# tinycdbff House Rules

Apply to every task in this repository unless explicitly overridden. Bias: caution
over speed on non-trivial work; use judgment on trivial tasks. Loaded automatically
at launch (same priority as `CLAUDE.md`). Subagents get their discipline from the
skills force-loaded via `briefing.skills`; this file is for the orchestrating agent.

## Engineering discipline

1. **Think before coding** — State assumptions; when uncertain, ask rather than
   guess. Push back when a simpler approach exists. Stop when confused; name what's
   unclear.
2. **Surgical changes** — This is a faithful port of upstream tinycdb. Touch only
   what the task needs; don't "improve" adjacent code, style or comments. A
   deviation from upstream must be deliberate and explained.
3. **Match the codebase** — Conformance over taste. The C style, the deliberate
   `cdb_pack` shift form, the terse variable names — keep them.
4. **A red result is a claim before it is a failure** — Before changing code to
   make a check pass, say what the check asserts and whether your fix keeps that
   claim. Reproduce a bug before fixing it; leave a regression test behind.
5. **Fail loud** — "Done" is wrong if anything was skipped. "Tests pass" is wrong
   if only the compile-check ran (see hazards).

## Delegation

Depends on whether the Agent/Task tool is available to you.

- **You can spawn subagents** (orchestrating main agent): Do NOT touch
  behaviour-relevant code yourself — delegate. Your lane: coordinate, inspect,
  plan, review diffs, run tests, edit prose docs. Only the `tinycdbff-*` agents get
  their skills force-loaded via `briefing.skills`; you get no briefing and would
  touch internals with too little context. When in doubt, delegate.

  | Task | Agent |
  |---|---|
  | Implement / refactor / debug behaviour-relevant code | `tinycdbff-worker` (default) |
  | Write / extend tests | `tinycdbff-test-writer` |
  | Commits, card → done | `tinycdbff-release-manager` |

  **Only `tinycdbff-release-manager` commits.** A worker hands its card to `review`
  and reports; you then dispatch the release-manager to cut the commit.

- **You cannot spawn subagents** (you ARE a `tinycdbff-*` agent): the delegation
  lock does not apply — implement, refactor, debug and test per these rules.

Behaviour-relevant = the C sources: runtime behaviour, the public API in `cdb.h`,
the on-disk format, error handling, and the test harness. Pure prose (README,
CLAUDE.md, this file) is not.

## Coordination — karr board

Ticket coordination is the orchestrating agent's job, so `karr` is always in scope
— just use the CLI, don't invoke the coordination skill first. Git-native kanban;
state lives in `refs/karr/*`; this repo has its own board.

- `karr list --compact` / `karr board` — open work · `karr show ID` — detail
- `karr create "Title" --priority high --tags a,b --body '…'` — new ticket
- `karr move ID in-progress --claim NAME` — start · `karr handoff ID --claim NAME --note "…"` — to review
- mutating commands auto-sync; `karr sync --pull|--push` for explicit exchange

Card life cycle: you claim and hand out → the worker notes and ends at `review` →
the release-manager commits and moves it to `done`. Full command surface: skill
`kanban-issues-karr-coordination`.

## Release & the public tracker — never without instruction

`sh t/compile-check.sh` and the behavioural tests are fine anytime. There is **no
package release** — tinycdbff is vendored source, nothing is built or published.
`git push` and tags are STRICTLY forbidden without the maintainer's explicit
go-ahead. The GitHub issue tracker (`cindustries/tinycdbff`) carries real users'
reports under the maintainer's name — **never read, comment on, close or create an
issue there on your own initiative**, only on explicit instruction about a specific
issue.

## Project hazards — the traps that make the wrong thing look right

- **compile-check is syntax-only.** `t/compile-check.sh` passing means the sources
  *compile* against an opaque stub, not that the index works. Broken logic and
  correct logic produce an identical green run. Never claim behaviour is verified
  on the compile-check alone — behaviour needs the in-memory FatFs shim.
- **FRESULT is never negative.** `f_lseek`/`f_read`/`f_write` return an enum
  (`FR_OK == 0`); `if (f_lseek(...) < 0)` is dead error handling that the stub and
  the compiler never flag. Check `!= FR_OK`. (Full detail: skill `tinycdbff-core`.)
- **The on-disk layout is a compatibility contract** — 2048-byte TOC, little-endian
  4-byte ints, djb hash. Changing the format silently breaks every reader.
- **Embedded target, no POSIX.** Don't introduce hosted-only headers (`<unistd.h>`
  etc.) or libc beyond `string.h`/`stdlib.h`/`errno.h`.
- **Public-domain header** stays on every source file; no incompatibly-licensed code.

## C specifics — reference, don't restate

The format contract, the FatFs seam, the API conventions, the portability rule and
the test model live in skill `tinycdbff-core` (force-loaded for `tinycdbff-*`
agents). Do not duplicate that content here.
