// Automation Forge as MCP tools. Adapter only.

#pragma once

#include "CoreMinimal.h"
#include "AFAuthoring.h"
#include "AFExecutor.h"
#include "AFLedger.h"
#include "AFNodeRegistry.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "AFToolset.generated.h"

/**
 * What the installed plugins can do, and what a pipeline may be built from.
 *
 * The node library is **found, not listed**. Every `BlueprintCallable` function on an editor subsystem
 * outside the engine becomes a node, as does anything marked `AFNode` anywhere - including a Blueprint
 * function library in somebody's own project. So the palette covers whatever is installed, ours and
 * theirs alike, and adding a plugin adds nodes with no edit to this one.
 */
UCLASS()
class AUTOMATIONFORGETOOLSET_API UAFToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:

	virtual FString GetToolsetVersion() const override { return TEXT("0.1"); }

	/**
	 * Which modules contribute nodes, and how many each.
	 *
	 * The cheapest way to see the library's shape, and the first thing to call when a function that
	 * should be a node is not showing up - a module missing here is a plugin that is not loaded.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Nodes")
	static TMap<FString, int32> GetNodeCategories();

	/**
	 * Search the node library.
	 *
	 * @param Category Restrict to one module, e.g. "SpeechForge". Empty for all.
	 * @param NameContains Substring of the function or display name. Empty for all.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Nodes")
	static TArray<FAFNodeType> FindNodes(const FString& Category, const FString& NameContains);

	/** One node's full signature - its pins, their types, and what it declares about cost and retries. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Nodes")
	static FAFNodeType GetNode(const FString& NodeTypeId);

	/**
	 * Re-walk the loaded classes and rebuild the library.
	 *
	 * Needed after enabling a plugin or compiling a Blueprint that adds nodes. Newly seen functions are
	 * minted a stable id, which is written to the project so a pipeline still resolves after a restart.
	 *
	 * @return how many node types exist afterwards.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Nodes")
	static int32 RediscoverNodes();

	// ---------------------------------------------------------------------------------------------
	// Ledger
	// ---------------------------------------------------------------------------------------------

	/**
	 * Cost a run before it commits: how much is already current, how much would be made, what it
	 * would spend.
	 *
	 * The moment the decision to spend can still be made. A node with no declared cost is reported as
	 * incomplete rather than counted as free, because a total that quietly treats unknown as zero is
	 * the number somebody approves a run on.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Ledger")
	static FAFPlan PlanRun(const TArray<FAFTargetKey>& Keys, FName Channel,
		const TArray<FString>& Hashes, double CostPerItem);

	/** Whether a slot already holds a result for these exact inputs. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Ledger")
	static bool IsCurrent(const FAFTargetKey& Key, FName Channel, const FString& InputHash);

	/** One slot, its candidate history and what is bound to it now. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Ledger")
	static FAFTarget GetTarget(const FAFTargetKey& Key, FName Channel);

	/** Record a produced result and bind it to its slot. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Ledger")
	static void RecordCandidate(const FAFTargetKey& Key, FName Channel, const FAFCandidate& Candidate);

	/** Bind an existing candidate - what a review gate does when a human picks one. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Ledger")
	static void SelectCandidate(const FAFTargetKey& Key, FName Channel, const FString& CandidateId);

	/** Mark a slot as hand-edited, so no pipeline overwrites it. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Ledger")
	static void SetGraduated(const FAFTargetKey& Key, FName Channel, bool bGraduated);

	/** The hash for a set of inputs. Order matters and is the caller's to fix. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Ledger")
	static FString MakeInputHash(const TArray<FString>& Parts);

	/** Every slot the ledger holds. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Ledger")
	static TArray<FAFTarget> GetAllTargets();

	// ---------------------------------------------------------------------------------------------
	// Pipelines
	// ---------------------------------------------------------------------------------------------

	/**
	 * Check a pipeline definition without running it, and without spending anything.
	 *
	 * Catches what is otherwise found halfway through a paid run: a node id that resolves to nothing,
	 * an input reading a channel no earlier step produces, an expand with no key. Call it before every
	 * run; Run calls it itself and refuses to start on an error.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Pipelines")
	static TArray<FAFCheck> CheckPipeline(const FString& PipelinePath);

	/**
	 * Start a run, and get its id back straight away.
	 *
	 * It does not run the pipeline here: the run advances itself from this point, a step at a time, and
	 * a step waiting on something slow keeps waiting long after this call has returned. Watch it with
	 * Get Pipeline Run. Refuses to start at all if the definition has errors, because a pipeline that
	 * stops halfway has usually spent money on the half it did.
	 *
	 * @param MaxItems Stop expanding beyond this many items. A guard rail for a first run against a
	 *        project nobody has measured, which is where an accidental four-hundred-item fan-out
	 *        comes from.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Pipelines")
	static FString StartPipelineRun(const FString& PipelinePath, int32 MaxItems);

	/**
	 * Where a run has got to.
	 *
	 * A run advances on its own; this is how you watch it. A status of Waiting means something slow is
	 * in flight and being polled - that is working, not stuck, and the poll count says how long it has
	 * been going.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Pipelines")
	static FAFRunState GetPipelineRun(const FString& RunId);

	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Pipelines")
	static TArray<FString> ListPipelineRuns();

	/**
	 * Wire one step's output into another step's input pin.
	 *
	 * **A wire is a binding.** Connecting a channel to a pin says exactly what typing `$channel` into
	 * it says - this exists so a pipeline written here and one drawn in the graph can express the same
	 * thing, not because there are two kinds of connection.
	 *
	 * Prefer this to Set Pipeline Binding for anything that reads an upstream channel: it is drawn as a
	 * wire, so somebody opening the graph can see where the data comes from.
	 *
	 * @param FromChannel Which of the upstream step's channels. Empty means its single output.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static bool ConnectPipelineSteps(
		const FString& PipelinePath, FName FromStep, FName FromChannel, FName ToStep, FName ToPin);

	/**
	 * Rename a step, taking its wires with it.
	 *
	 * Worth doing whenever a step is called something only its author understands. The id is what
	 * appears on the node, in every log line and in every decision a gate raises, so a pipeline of
	 * `step2` and `filter3` is one nobody else can read.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static bool RenamePipelineStep(const FString& PipelinePath, FName StepId, FName NewStepId);

	/** Unwire a pin. It falls back to whatever value is typed on it, if any. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static bool DisconnectPipelineStep(const FString& PipelinePath, FName ToStep, FName ToPin);

	/**
	 * Lay a pipeline out left to right, in columns by how far downstream each step is.
	 *
	 * Worth calling after building one, so it opens readable rather than as a pile at the origin. It
	 * replaces the current arrangement, so do not call it on a pipeline somebody has arranged by hand.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static bool ArrangePipeline(const FString& PipelinePath);

	/** Stop a run. What it produced stays; what it had not started never will. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Pipelines")
	static bool CancelPipelineRun(const FString& RunId);

	/**
	 * What a suspended run is waiting for somebody to decide.
	 *
	 * A run stops at a gate when something needs a judgement rather than a rule - typically *this is
	 * too far gone to correct, so it should be regenerated instead*. Each entry says which item, what
	 * was tested, what was found, and what the pipeline's author wanted asked.
	 *
	 * **Do not answer these on the user's behalf.** A gate exists because somebody wanted to look:
	 * report what is waiting, and let them decide.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Pipelines")
	static TArray<FAFDecision> GetPendingDecisions(const FString& RunId);

	/**
	 * Answer a gate, once the user has told you what they want.
	 *
	 * @param ItemKey One decision, as it appears in the pending list. Empty answers every decision that
	 *        step raised - right when the user has looked at a report and given one answer.
	 * @param bApprove Let it through, or hold it back. **Approving is a decision to spend**: the steps
	 *        after a gate are the expensive ones, which is why the gate is there.
	 * @param Note Why, for whoever reads this run later.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Pipelines")
	static bool AnswerPipelineGate(
		const FString& RunId, FName StepId, const FString& ItemKey, bool bApprove, const FString& Note);

	/**
	 * Start a run and wait for it. **Blocks** - for a batch or a test, not for interactive use.
	 *
	 * Prefer starting a run and watching it. This exists because a headless batch has nothing else to
	 * do, and a test that polls by hand tests the polling rather than the pipeline.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Pipelines")
	static FAFRunState RunPipelineToCompletion(const FString& PipelinePath, int32 MaxItems, double TimeoutSeconds);

	// ---------------------------------------------------------------------------------------------
	// Authoring
	//
	// A pipeline is data, so it can be written here without a graph existing. That is not a stopgap
	// until the editor arrives - it is the rule the whole design rests on, because a definition that
	// could only be drawn could not be written by an agent at all.
	// ---------------------------------------------------------------------------------------------

	/** Every pipeline in the project. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static TArray<FString> ListPipelines();

	/** Create an empty pipeline. @return its asset path. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static FString CreatePipeline(const FString& PackagePath, const FString& AssetName, const FString& Description);

	/**
	 * Append a step that runs a node over the items at one grain.
	 *
	 * The ordinary step, and the one most pipelines are mostly made of: generating, solving,
	 * ingesting, reporting. It adds channels to each item it sees and never rewrites one.
	 *
	 * Caching, output paths and waiting are **not** here. They are real and they matter, and a step
	 * that needs one says so with the matching Set Step call - which is what keeps this signature the
	 * size of the thing it describes.
	 *
	 * @param StepId One word, stable. It is what a wire names, what a log line says, and what a gate
	 *        files a decision under, so renaming one later is a real edit rather than a relabel.
	 * @param Node   The node, as `Category  -  Class:Function` - the same string the node library
	 *        prints. Resolved to a stable id when the step is added, and it is the id that survives a
	 *        rename of the function behind it.
	 * @param Grain  Which level this runs at. Leave it None and the node's own declaration is used,
	 *        which is right almost always.
	 * @param Inputs Values for the node's pins. `$channel` reads a channel off the item, looking up
	 *        the hierarchy; anything else is a literal. Pass none and every pin takes its default.
	 * @param EmitsChannel Where this node's return value is written. None means the step is run for
	 *        its effect and produces nothing worth carrying forward.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void AddPipelineStep(
		const FString& PipelinePath, FName StepId, const FString& Node, FName Grain,
		const TArray<FAFBinding>& Inputs, FName EmitsChannel);

	/**
	 * Append a step that turns each item into many - a project into its dialogues, a dialogue into its
	 * lines.
	 *
	 * @param EmitsGrain The level the produced items sit at.
	 * @param KeyFrom **The most consequential field in a definition.** Which of the produced channels
	 *        form each item's key, in order. Build it from stable natural ids the source already
	 *        guarantees unique - a dialogue's own node ids - and a second run finds the same items.
	 *        Build it from a display name, an index, or anything containing the content itself, and
	 *        the second run mints hundreds of fresh items and pays for all of them again. Nothing
	 *        errors; the only symptom is the bill.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void AddPipelineExpand(
		const FString& PipelinePath, FName StepId, const FString& Node, FName Grain,
		const TArray<FAFBinding>& Inputs, FName EmitsGrain, const TArray<FName>& KeyFrom);

	/**
	 * Append a filter: narrow the items at one grain, leaving every other level untouched.
	 *
	 * **This is how a run is scoped, and scoping is not optional on a shared project.** A source step
	 * is unscoped by nature - left to itself it reads everything, including a marketplace plugin's
	 * demo content, which is exactly the material that must not be touched. `Starts With
	 * /Game/_EP1/` keeps one episode; the same test inverted drops a vendor folder.
	 *
	 * Narrowing one grain never disturbs another, because dropping a dialogue for one bad line would
	 * take every other line of it too.
	 *
	 * @param WhereChannel The channel to test. Call Describe Pipeline for what is readable here.
	 * @param Value        What to compare against. Ignored by Has Value.
	 * @param bInvert      Keep the items that do *not* match instead.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void AddPipelineFilter(
		const FString& PipelinePath, FName StepId, FName Grain,
		FName WhereChannel, EAFMatch Match, const FString& Value, bool bInvert = false);

	/**
	 * Append a gate: stop for a person.
	 *
	 * The one construct that is not about data. A gate tests the same way a filter does and then,
	 * instead of dropping what fails, records a decision and waits - so the expensive steps after it
	 * do not run until somebody has said they should.
	 *
	 * **Nothing here opens a dialog.** A suspended run is a file with questions attached, answerable
	 * from the graph, from an agent, or from a commandlet, days later and on another machine.
	 *
	 * @param Question What to ask, written for somebody arriving at a suspended run with no memory of
	 *        the pipeline. Say what failed and what the choices are.
	 * @param GateAction Hold just the failing items and let the rest carry on, or stop the whole run.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void AddPipelineGate(
		const FString& PipelinePath, FName StepId, FName Grain,
		FName WhereChannel, EAFMatch Match, const FString& Value, const FString& Question,
		EAFGateAction GateAction = EAFGateAction::HoldItems, bool bInvert = false);

	/**
	 * Change what a filter or a gate tests, without rebuilding it.
	 *
	 * Correcting a test used to mean removing the step - which orphans the wires into it - and adding
	 * it back with every other field respecified.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void SetStepTest(
		const FString& PipelinePath, FName StepId,
		FName WhereChannel, EAFMatch Match, const FString& Value, bool bInvert = false);

	/**
	 * Cache this step's result, and refuse to overwrite work nobody recorded making.
	 *
	 * Set this on anything that costs money or takes minutes. The item is hashed from the node, its
	 * resolved inputs and whatever Hash Also names; if the ledger already holds that hash, the node is
	 * not called at all.
	 *
	 * @param LedgerChannel The channel to cache under. None means run every time, which is right for
	 *        anything free and fast.
	 * @param HashAlso **Where a provider and its model version belong.** A generator usually takes
	 *        neither as an input - the voice asset decides them - so without naming them here,
	 *        swapping model leaves the hash unchanged and the cache serves the old result for ever.
	 * @param GuardChannel Point this at the channel naming what is already there - the sound a line
	 *        already has - and the step skips any item where that channel has a value and the ledger
	 *        holds no history for the slot. The reasoning is simply *there is already something here
	 *        and we did not make it*. None means no guard, which is right for a step that only reads.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void SetStepCaching(
		const FString& PipelinePath, FName StepId,
		FName LedgerChannel, const TArray<FName>& HashAlso, FName GuardChannel);

	/**
	 * Where this step's product goes and what it is called.
	 *
	 * Empty leaves the pipeline's own root, which leaves the project's. Set it where one step's output
	 * does not belong beside the others - a quality report is not an episode's content, and a
	 * corrected clip is not the take it was corrected from.
	 *
	 * The resolved path is published as the channel `outputPath` rather than forced onto whatever pin
	 * looked path-shaped, so **a pin has to read `$outputPath` for this to do anything**. The checker
	 * says so when nothing does.
	 *
	 * @param NamePattern `{name}` is what the step would have called it; everything else is literal.
	 *        `{name}_postProcessed` is what tells a derived asset from what it derives from, which a
	 *        folder alone does not.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void SetStepOutput(
		const FString& PipelinePath, FName StepId, const FString& OutputPath, const FString& NamePattern);

	/**
	 * How this step knows the slow work it started has finished.
	 *
	 * Only for a node that starts work rather than doing it. Such a node declares its own answer and a
	 * step inherits it, so this is for the case where one pipeline's context means something different
	 * - and getting it wrong is quiet in both directions: a step that waits for nothing runs the next
	 * one on top of work still in flight, and a step watching a field that does not exist waits out
	 * its whole timeout and then blames the provider. Both are refused before a run starts.
	 *
	 * @param DoneWhenChannel The field on the status node's report to watch, with the value that means
	 *        done - `bRunning` and `False` for a face solve.
	 * @param StatusInputs Only where the step's own inputs and the channel names do not reach the
	 *        status node's pins. Rarely needed.
	 * @param TimeoutSeconds Give up after this long. Zero keeps the step's own generous default.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void SetStepWait(
		const FString& PipelinePath, FName StepId,
		FName DoneWhenChannel, const FString& DoneWhenEquals,
		const TArray<FAFBinding>& StatusInputs, double TimeoutSeconds = 0.0);

	/**
	 * Override the shape a step runs in, where the node's own declaration is wrong for this pipeline.
	 *
	 * Almost never needed: a node says whether it expands, maps or reduces, and it is what knows. This
	 * exists because the graph's details panel has always let somebody change it, and a fact settable
	 * in one surface and not the other is two pipelines wearing one name.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void SetStepShape(const FString& PipelinePath, FName StepId, EAFShape Shape);

	/**
	 * Where this pipeline's products go, and what it is for.
	 *
	 * The output root is the pipeline's answer to a question the plugin would otherwise answer
	 * project-wide - which is how quality-check clips generated for a test once landed in an episode's
	 * content folder. A step may override it again.
	 *
	 * Both were editable in the graph and settable by nothing else.
	 *
	 * @param OutputPath A content path such as `/Game/_Generated/QC`. It must be under a mounted root:
	 *        content written somewhere unmounted is created in memory, never saved, and reported as a
	 *        success. Empty leaves each plugin writing to its own configured root.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void SetPipelineOutput(const FString& PipelinePath, const FString& OutputPath);

	/** What this pipeline is for, in a sentence, for whoever opens it next. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void SetPipelineDescription(const FString& PipelinePath, const FString& Description);

	/**
	 * Clear whatever drives one pin, wire or literal, so it falls back to the node's own default.
	 *
	 * Disconnect only ever removed a wire, so a mistyped literal could be corrected but never removed -
	 * and the error it gave said nothing drives that pin, which was true of wires and false of the pin.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void ClearStepBinding(const FString& PipelinePath, FName StepId, FName Pin);

	/**
	 * Every node in the library as one line each: `Category  -  Class:Function`.
	 *
	 * Start here. Find Nodes returns every field of every node and is tens of thousands of characters,
	 * which is the wrong thing to read when the question is what exists; ask this first and Get Node
	 * for the one you want.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Discovery")
	static TArray<FString> ListNodeNames(const FString& Category);

	/** A note that travels with the pipeline. Costs nothing and is the first thing missed. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void SetStepComment(const FString& PipelinePath, FName StepId, const FString& Comment);

	/**
	 * What this pipeline would cost, without running it and without writing anything.
	 *
	 * **This is the call to make between checking a definition and starting it**, and the number to put
	 * in front of whoever is paying before anything metered runs.
	 *
	 * It walks the definition for real: steps that declare themselves read-only are executed, because
	 * the only way to know how many lines an episode has is to read them. At the first step that would
	 * write, spend or wait, it counts what that step would do to each item - against the ledger, so
	 * anything already current is excluded, and against the guard, so anything somebody else made is
	 * excluded - works out the cost, and stops. Where it stopped and why come back with the answer.
	 *
	 * **Read `costIncomplete` before quoting the total.** True means the number is a floor: either the
	 * walk stopped before the end, or a node that would run has never declared what it charges. A node
	 * that has said nothing about its cost is never counted as free.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Pipelines")
	static FAFPipelinePlan PlanPipelineRun(const FString& PipelinePath, int32 MaxItems = 1000);

	/** Set one input binding. `$channel` reads a channel; anything else is a literal. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void SetPipelineBinding(const FString& PipelinePath, FName StepId, FName Pin, const FString& Value);

	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void RemovePipelineStep(const FString& PipelinePath, FName StepId);

	/** Move a step. Execution order is the structure, so this is how a mistake in it is corrected. */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static void MovePipelineStep(const FString& PipelinePath, FName StepId, int32 NewIndex);

	/**
	 * Read a pipeline back, with **the channels readable at every step**.
	 *
	 * Call this before adding a step. What can be bound at a given point depends on which earlier
	 * steps emit and at what grain, and it is not answerable by reading the definition - guessing
	 * produces a binding that resolves to nothing, and a node called with an empty input usually
	 * succeeds at doing nothing.
	 */
	UFUNCTION(meta = (AICallable), Category = "AutomationForge|Authoring")
	static TArray<FAFStepView> DescribePipeline(const FString& PipelinePath);

private:

	static UAFNodeRegistry* GetRegistryChecked();
	static UAFAuthoring* GetAuthoringChecked();
	static UAFExecutor* GetExecutorChecked();
	static const UAFPipeline* LoadPipeline(const FString& PipelinePath);
	static UAFLedger* GetLedgerChecked();
};
