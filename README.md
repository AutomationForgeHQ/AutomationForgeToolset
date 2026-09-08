# Automation Forge Toolset

Exposes [Automation Forge](https://kovati.dev/plugins/automationforge/) and
[Automation Forge Pipelines](https://kovati.dev/plugins/automationforge/) as native Model Context Protocol
tools, so an agent can read the node library, author a pipeline, run it, and answer the gates it
stops at — through exactly the operations a person uses.

**Status: 0.1.** The node library, the ledger and the pipeline authoring and run surfaces are
exposed and exercised; the executor behind them is prototype quality.

Same split as every other toolset in the family, and for the same reason: `ToolsetRegistry` and
`ModelContextProtocol` are **Experimental** engine plugins, and folding this in would make
Automation Forge refuse to load anywhere they are turned off. Delete this plugin and Automation
Forge behaves identically.

This module holds **no logic**. Every tool forwards to a subsystem.

---

## Tools

**The node library** — what a pipeline can be built out of.

| Tool | |
|---|---|
| `GetNodeCategories`, `ListNodeNames` | What exists |
| `FindNodes`, `GetNode` | Search it, and read one node's signature |
| `RediscoverNodes` | Re-scan after a plugin is enabled |

**The ledger** — what was made, what it cost, and what is bound to each slot.

| Tool | |
|---|---|
| `GetAllTargets`, `GetTarget` | Every generated slot, and one of them |
| `IsCurrent`, `MakeInputHash` | Whether a slot's inputs still match what produced it |
| `RecordCandidate`, `SelectCandidate` | Add a take; choose the one that ships |
| `SetGraduated` | Hand-authored from here — the tooling stops touching this slot |
| `PlanRun` | Cost a run before it commits: what's already current, what would be made, what it would spend |

**Pipelines** — author one, run it, and stand at the gates.

| Tool | |
|---|---|
| `ListPipelines`, `CreatePipeline`, `SetPipelineDescription`, `SetPipelineOutput` | The definition |
| `AddPipelineStep`, `AddPipelineExpand`, `AddPipelineFilter`, `AddPipelineGate` | Its steps |
| `RemovePipelineStep` | Remove a step |
| `MovePipelineStep` | Move a step — execution order is the structure |
| `ConnectPipelineSteps`, `DisconnectPipelineStep`, `RenamePipelineStep`, `SetStepComment` | Wiring |
| `SetPipelineBinding` | Set one input binding — `$channel` reads a channel, anything else is a literal |
| `SetStepTest`, `SetStepCaching`, `SetStepOutput`, `SetStepWait`, `SetStepShape`, `ClearStepBinding` | Per-step behaviour |
| `ArrangePipeline` | Lay the graph out — coordinates are never passed in |
| `DescribePipeline` | Read a pipeline back, with the channels readable at every step |
| `CheckPipeline` | Validate before spending anything |
| `PlanPipelineRun` | What a pipeline would cost, without running it and without writing anything |
| `StartPipelineRun`, `RunPipelineToCompletion`, `CancelPipelineRun` | Run it |
| `GetPipelineRun`, `ListPipelineRuns` | Watch it |
| `GetPendingDecisions`, `AnswerPipelineGate` | A pipeline stops where a person has to decide |

---

## The rule this obeys

A pipeline **never spends money without being asked**, and a gate is where it waits. `CheckPipeline`
is free and says what a run would do; a run that finds its output current executes nothing. Both of
those are the point rather than optimisations.

---

Part of the [Automation Forge](https://github.com/AutomationForgeHQ) suite for Unreal Engine 5.8.
Licensed under Apache-2.0.
