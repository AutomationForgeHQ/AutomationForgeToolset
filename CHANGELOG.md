# AutomationForgeToolset

Every released version of AutomationForgeToolset, newest first. A release publishes **one** section of this
file — the one whose heading matches its tag — as its release notes; for an `open` plugin those
notes are posted to Discord `#releases` automatically. Write for someone who installs the plugin,
not for the commit log.

Headings are `## <x.y.z> — <date>`. Use `Added` / `Changed` / `Fixed` / `Compatibility` /
`Known issues`, only the ones that apply.

## 0.1.2 — 2026-10-04

### Changed
- The version this toolset reports to an agent is now read from the plugin's own
  descriptor rather than repeated in C++, so it can no longer answer a number the
  installed package does not carry.
- Copyright and licence notices now name Bojan Andrejek / MetaWorx LLC. It is still Apache 2.0, and nothing about how you may use it changed.

## 0.1.1 — 2026-09-08
- Packaging fix: the release now carries everything the register allows. `BuildPlugin`'s filter excludes `Config/` and every `public_extra` path, so earlier zips shipped without them.
- The package now carries its Apache-2.0 LICENSE, which earlier zips omitted.
- `GetToolsetVersion()` answers this plugin's real version; it had drifted from the descriptor.

## 0.1.0 — 2026-08-28
- A node library found by reflection, not by a list
- A ledger that knows what already exists, and what a run would cost
- A pipeline that runs from data, and a register of what is still wrong
- A pipeline an agent can write without a graph existing
- A run that outlives the call that started it
- The wait, proven against a solve that really takes minutes
- A gate: the one construct that is not about data
- Wires you can see, undo on the keyboard, and assets you pick
- A details panel you can fill in, and nodes with names
- A step needs what a step needs, and not twenty other things
- Cost before commit, for real this time
