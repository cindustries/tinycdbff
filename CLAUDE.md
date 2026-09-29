# tinycdbff

A public-domain port of Michael Tokarev's **TinyCDB** (a constant database, based
on djb's cdb) to **Chan FatFs**. It is a *source-drop module*: consumers vendor the
`cdb_*.c` files and link them against a real Chan FatFs that supplies `ff.h`.
tinycdbff itself is never built, installed or published.

- **Public API**: `cdb.h` — reader (`cdb_seek` / `cdb_bread`) and writer
  (`cdb_make_start` / `cdb_make_add` / `cdb_make_put` / `cdb_make_finish`).
- **Tests**: `sh t/compile-check.sh` syntax-checks every source against the opaque
  FatFs stub `t/ff.h` (also run in CI). It proves the sources compile, not that cdb
  behaves — behaviour needs the in-memory FatFs shim.

The architecture, the on-disk format contract, the FatFs I/O seam and its traps
live in skill `tinycdbff-core`, not here.

## Delegation

Delegate behaviour-relevant code to the right agent instead of touching it yourself
— the principle and the lane definitions are in `.claude/rules/tinycdbff-rules.md`
(auto-loaded every turn).

| Task | Agent |
|---|---|
| Implement / refactor / debug behaviour-relevant code | `tinycdbff-worker` (default) |
| Write / extend tests | `tinycdbff-test-writer` |
| Commits, card → done | `tinycdbff-release-manager` |

The agents carry their knowledge via `briefing.skills` (see `.claude/agents/`); the
main agent delegates rather than loading those skills itself. **Only the
release-manager commits** — workers and the test-writer leave a commit-ready tree
and a report.

## Skills & coordination

Shared skills are managed by **skilletor** (`.claude/skilletor.json`, sources
`getty` and `karr`) and materialised under `.claude/skills/`; the repo-owned skill
`tinycdbff-core` lives there too. Cross-session work is tracked on the repo's
**karr** board (`karr board`). Don't hand-edit a skilletor-managed skill — change it
in its source and re-`sync`.
