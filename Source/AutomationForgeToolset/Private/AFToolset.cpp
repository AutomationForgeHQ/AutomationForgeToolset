#include "AFToolset.h"

#include "Editor.h"
#include "Kismet/KismetSystemLibrary.h"

UAFNodeRegistry* UAFToolset::GetRegistryChecked()
{
	UAFNodeRegistry* Registry = GEditor ? GEditor->GetEditorSubsystem<UAFNodeRegistry>() : nullptr;

	if (!Registry)
	{
		UKismetSystemLibrary::RaiseScriptError(
			TEXT("Automation Forge is not available. The plugin is editor-only - check it is enabled."));
	}

	return Registry;
}

TMap<FString, int32> UAFToolset::GetNodeCategories()
{
	UAFNodeRegistry* Registry = GetRegistryChecked();
	return Registry ? Registry->GetCategoryCounts() : TMap<FString, int32>();
}

TArray<FAFNodeType> UAFToolset::FindNodes(const FString& Category, const FString& NameContains)
{
	UAFNodeRegistry* Registry = GetRegistryChecked();
	return Registry ? Registry->FindNodeTypes(Category, NameContains) : TArray<FAFNodeType>();
}

FAFNodeType UAFToolset::GetNode(const FString& NodeTypeId)
{
	UAFNodeRegistry* Registry = GetRegistryChecked();
	if (!Registry)
	{
		return FAFNodeType();
	}

	FAFNodeType Node;
	if (!Registry->GetNodeType(NodeTypeId, Node))
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(
			TEXT("No node type '%s'. Call Find Nodes to list what exists - ids are stable per project ")
			TEXT("and are not function names."), *NodeTypeId));
	}

	return Node;
}

int32 UAFToolset::RediscoverNodes()
{
	UAFNodeRegistry* Registry = GetRegistryChecked();
	return Registry ? Registry->DiscoverNodes() : 0;
}

// -------------------------------------------------------------------------------------------------
// Ledger
// -------------------------------------------------------------------------------------------------

UAFLedger* UAFToolset::GetLedgerChecked()
{
	UAFLedger* Ledger = GEditor ? GEditor->GetEditorSubsystem<UAFLedger>() : nullptr;

	if (!Ledger)
	{
		UKismetSystemLibrary::RaiseScriptError(
			TEXT("Automation Forge is not available. The plugin is editor-only - check it is enabled."));
	}

	return Ledger;
}

FAFPlan UAFToolset::PlanRun(
	const TArray<FAFTargetKey>& Keys, FName Channel, const TArray<FString>& Hashes, double CostPerItem)
{
	UAFLedger* Ledger = GetLedgerChecked();
	return Ledger ? Ledger->PlanRun(Keys, Channel, Hashes, CostPerItem) : FAFPlan();
}

bool UAFToolset::IsCurrent(const FAFTargetKey& Key, FName Channel, const FString& InputHash)
{
	UAFLedger* Ledger = GetLedgerChecked();
	return Ledger ? Ledger->IsCurrent(Key, Channel, InputHash) : false;
}

FAFTarget UAFToolset::GetTarget(const FAFTargetKey& Key, FName Channel)
{
	UAFLedger* Ledger = GetLedgerChecked();
	return Ledger ? Ledger->GetTarget(Key, Channel) : FAFTarget();
}

void UAFToolset::RecordCandidate(const FAFTargetKey& Key, FName Channel, const FAFCandidate& Candidate)
{
	UAFLedger* Ledger = GetLedgerChecked();
	if (!Ledger)
	{
		return;
	}

	FString Error;
	if (!Ledger->RecordCandidate(Key, Channel, Candidate, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

void UAFToolset::SelectCandidate(const FAFTargetKey& Key, FName Channel, const FString& CandidateId)
{
	UAFLedger* Ledger = GetLedgerChecked();
	if (!Ledger)
	{
		return;
	}

	FString Error;
	if (!Ledger->SelectCandidate(Key, Channel, CandidateId, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

void UAFToolset::SetGraduated(const FAFTargetKey& Key, FName Channel, bool bGraduated)
{
	UAFLedger* Ledger = GetLedgerChecked();
	if (!Ledger)
	{
		return;
	}

	FString Error;
	if (!Ledger->SetGraduated(Key, Channel, bGraduated, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

FString UAFToolset::MakeInputHash(const TArray<FString>& Parts)
{
	return UAFLedger::MakeInputHash(Parts);
}

TArray<FAFTarget> UAFToolset::GetAllTargets()
{
	UAFLedger* Ledger = GetLedgerChecked();
	return Ledger ? Ledger->GetAllTargets() : TArray<FAFTarget>();
}

// -------------------------------------------------------------------------------------------------
// Pipelines
// -------------------------------------------------------------------------------------------------

UAFExecutor* UAFToolset::GetExecutorChecked()
{
	UAFExecutor* Executor = GEditor ? GEditor->GetEditorSubsystem<UAFExecutor>() : nullptr;

	if (!Executor)
	{
		UKismetSystemLibrary::RaiseScriptError(
			TEXT("The pipelines plugin is not available - check AutomationForgePipelines is enabled."));
	}

	return Executor;
}

const UAFPipeline* UAFToolset::LoadPipeline(const FString& PipelinePath)
{
	const UAFPipeline* Pipeline = LoadObject<UAFPipeline>(nullptr, *PipelinePath);

	if (!Pipeline)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(
			TEXT("No pipeline at '%s'."), *PipelinePath));
	}

	return Pipeline;
}

TArray<FAFCheck> UAFToolset::CheckPipeline(const FString& PipelinePath)
{
	UAFExecutor* Executor = GetExecutorChecked();
	const UAFPipeline* Pipeline = LoadPipeline(PipelinePath);

	return Executor && Pipeline ? Executor->Check(Pipeline) : TArray<FAFCheck>();
}

FString UAFToolset::StartPipelineRun(const FString& PipelinePath, int32 MaxItems)
{
	UAFExecutor* Executor = GetExecutorChecked();
	const UAFPipeline* Pipeline = LoadPipeline(PipelinePath);

	if (!Executor || !Pipeline)
	{
		return FString();
	}

	const FString RunId = Executor->StartRun(Pipeline, MaxItems);

	if (RunId.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(
			TEXT("The definition has errors, so nothing was started and nothing was spent. Call Check "
				"Pipeline to see them."));
	}

	return RunId;
}

FAFRunState UAFToolset::GetPipelineRun(const FString& RunId)
{
	UAFExecutor* Executor = GetExecutorChecked();
	return Executor ? Executor->GetRun(RunId) : FAFRunState();
}

TArray<FString> UAFToolset::ListPipelineRuns()
{
	UAFExecutor* Executor = GetExecutorChecked();
	return Executor ? Executor->ListRuns() : TArray<FString>();
}

bool UAFToolset::CancelPipelineRun(const FString& RunId)
{
	UAFExecutor* Executor = GetExecutorChecked();
	return Executor ? Executor->CancelRun(RunId) : false;
}

FAFRunState UAFToolset::RunPipelineToCompletion(
	const FString& PipelinePath, int32 MaxItems, double TimeoutSeconds)
{
	UAFExecutor* Executor = GetExecutorChecked();
	const UAFPipeline* Pipeline = LoadPipeline(PipelinePath);

	return Executor && Pipeline
		? Executor->RunToCompletion(Pipeline, MaxItems, TimeoutSeconds)
		: FAFRunState();
}

// -------------------------------------------------------------------------------------------------
// Authoring
// -------------------------------------------------------------------------------------------------

UAFAuthoring* UAFToolset::GetAuthoringChecked()
{
	UAFAuthoring* Authoring = GEditor ? GEditor->GetEditorSubsystem<UAFAuthoring>() : nullptr;

	if (!Authoring)
	{
		UKismetSystemLibrary::RaiseScriptError(
			TEXT("The pipelines plugin is not available - check AutomationForgePipelines is enabled."));
	}

	return Authoring;
}

TArray<FString> UAFToolset::ListPipelines()
{
	UAFAuthoring* Authoring = GetAuthoringChecked();
	return Authoring ? Authoring->ListPipelines() : TArray<FString>();
}

FString UAFToolset::CreatePipeline(
	const FString& PackagePath, const FString& AssetName, const FString& Description)
{
	UAFAuthoring* Authoring = GetAuthoringChecked();
	if (!Authoring)
	{
		return FString();
	}

	FString Error;
	const FString Path = Authoring->CreatePipeline(PackagePath, AssetName, Description, Error);

	if (Path.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}

	return Path;
}

// One construct per tool, and each one's parameters are only what that construct needs.
//
// This was a single call taking the whole step struct, which reads well and published badly: the
// registry marks every field of a USTRUCT parameter required, because Unreal can only mark a
// *function parameter* optional and only then when it has a non-empty default. So adding a two-field
// filter meant sending gate actions, async timeouts and status inputs it had no opinion about -
// twenty-three fields for a step that needed four. Splitting by construct is what makes the schema
// tell the truth about what is needed, and it costs nothing underneath: all four still build a spec
// and hand it to the same AddStep.
namespace
{
	void AddSpec(const FString& PipelinePath, const FAFStepSpec& Spec)
	{
		UAFAuthoring* Authoring = GEditor ? GEditor->GetEditorSubsystem<UAFAuthoring>() : nullptr;

		if (!Authoring)
		{
			UKismetSystemLibrary::RaiseScriptError(
				TEXT("The pipelines plugin is not available - check AutomationForgePipelines is enabled."));
			return;
		}

		FString Error;
		if (!Authoring->AddStep(PipelinePath, Spec, Error))
		{
			UKismetSystemLibrary::RaiseScriptError(Error);
		}
	}

	void EditStep(const FString& PipelinePath, FName StepId, TFunctionRef<void(FAFStep&)> Edit)
	{
		UAFAuthoring* Authoring = GEditor ? GEditor->GetEditorSubsystem<UAFAuthoring>() : nullptr;

		if (!Authoring)
		{
			UKismetSystemLibrary::RaiseScriptError(
				TEXT("The pipelines plugin is not available - check AutomationForgePipelines is enabled."));
			return;
		}

		FString Error;
		if (!Authoring->EditStep(PipelinePath, StepId, Edit, Error))
		{
			UKismetSystemLibrary::RaiseScriptError(Error);
		}
	}
}

void UAFToolset::AddPipelineStep(
	const FString& PipelinePath, FName StepId, const FString& Node, FName Grain,
	const TArray<FAFBinding>& Inputs, FName EmitsChannel)
{
	FAFStepSpec Spec;

	Spec.StepId = StepId;
	Spec.Construct = EAFConstruct::Node;
	Spec.Node = Node;
	Spec.Grain = Grain;
	Spec.Inputs = Inputs;
	Spec.EmitsChannel = EmitsChannel;

	AddSpec(PipelinePath, Spec);
}

void UAFToolset::AddPipelineExpand(
	const FString& PipelinePath, FName StepId, const FString& Node, FName Grain,
	const TArray<FAFBinding>& Inputs, FName EmitsGrain, const TArray<FName>& KeyFrom)
{
	FAFStepSpec Spec;

	Spec.StepId = StepId;
	Spec.Construct = EAFConstruct::Node;
	Spec.Shape = EAFShape::Expand;
	Spec.Node = Node;
	Spec.Grain = Grain;
	Spec.Inputs = Inputs;
	Spec.EmitsGrain = EmitsGrain;
	Spec.KeyFrom = KeyFrom;

	AddSpec(PipelinePath, Spec);
}

void UAFToolset::AddPipelineFilter(
	const FString& PipelinePath, FName StepId, FName Grain,
	FName WhereChannel, EAFMatch Match, const FString& Value, bool bInvert)
{
	FAFStepSpec Spec;

	Spec.StepId = StepId;
	Spec.Construct = EAFConstruct::Filter;
	Spec.Grain = Grain;
	Spec.WhereChannel = WhereChannel;
	Spec.Match = Match;
	Spec.Value = Value;
	Spec.bInvert = bInvert;

	AddSpec(PipelinePath, Spec);
}

void UAFToolset::AddPipelineGate(
	const FString& PipelinePath, FName StepId, FName Grain,
	FName WhereChannel, EAFMatch Match, const FString& Value, const FString& Question,
	EAFGateAction GateAction, bool bInvert)
{
	FAFStepSpec Spec;

	Spec.StepId = StepId;
	Spec.Construct = EAFConstruct::Gate;
	Spec.Grain = Grain;
	Spec.WhereChannel = WhereChannel;
	Spec.Match = Match;
	Spec.Value = Value;
	Spec.Question = Question;
	Spec.GateAction = GateAction;
	Spec.bInvert = bInvert;

	AddSpec(PipelinePath, Spec);
}

void UAFToolset::SetStepTest(
	const FString& PipelinePath, FName StepId,
	FName WhereChannel, EAFMatch Match, const FString& Value, bool bInvert)
{
	EditStep(PipelinePath, StepId, [&](FAFStep& Step)
	{
		Step.WhereChannel = WhereChannel;
		Step.Match = Match;
		Step.Value = Value;
		Step.bInvert = bInvert;
	});
}

void UAFToolset::SetStepCaching(
	const FString& PipelinePath, FName StepId,
	FName LedgerChannel, const TArray<FName>& HashAlso, FName GuardChannel)
{
	EditStep(PipelinePath, StepId, [&](FAFStep& Step)
	{
		Step.LedgerChannel = LedgerChannel;
		Step.HashAlso = HashAlso;
		Step.GuardChannel = GuardChannel;
	});
}

void UAFToolset::SetStepOutput(
	const FString& PipelinePath, FName StepId, const FString& OutputPath, const FString& NamePattern)
{
	EditStep(PipelinePath, StepId, [&](FAFStep& Step)
	{
		Step.OutputPath = OutputPath;
		Step.NamePattern = NamePattern;
	});
}

void UAFToolset::SetStepWait(
	const FString& PipelinePath, FName StepId,
	FName DoneWhenChannel, const FString& DoneWhenEquals,
	const TArray<FAFBinding>& StatusInputs, double TimeoutSeconds)
{
	EditStep(PipelinePath, StepId, [&](FAFStep& Step)
	{
		Step.DoneWhenChannel = DoneWhenChannel;
		Step.DoneWhenEquals = DoneWhenEquals;
		Step.StatusInputs = StatusInputs;

		// Zero keeps whatever the step already had, which is the node's generous default. Writing zero
		// through would make every step that did not mention a timeout time out instantly.
		if (TimeoutSeconds > 0.0)
		{
			Step.TimeoutSeconds = TimeoutSeconds;
		}
	});
}

void UAFToolset::SetStepComment(const FString& PipelinePath, FName StepId, const FString& Comment)
{
	EditStep(PipelinePath, StepId, [&](FAFStep& Step)
	{
		Step.Comment = Comment;
	});
}

void UAFToolset::SetPipelineBinding(
	const FString& PipelinePath, FName StepId, FName Pin, const FString& Value)
{
	UAFAuthoring* Authoring = GetAuthoringChecked();
	if (!Authoring)
	{
		return;
	}

	FString Error;
	if (!Authoring->SetBinding(PipelinePath, StepId, Pin, Value, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

void UAFToolset::RemovePipelineStep(const FString& PipelinePath, FName StepId)
{
	UAFAuthoring* Authoring = GetAuthoringChecked();
	if (!Authoring)
	{
		return;
	}

	FString Error;
	if (!Authoring->RemoveStep(PipelinePath, StepId, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

void UAFToolset::MovePipelineStep(const FString& PipelinePath, FName StepId, int32 NewIndex)
{
	UAFAuthoring* Authoring = GetAuthoringChecked();
	if (!Authoring)
	{
		return;
	}

	FString Error;
	if (!Authoring->MoveStep(PipelinePath, StepId, NewIndex, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

TArray<FAFStepView> UAFToolset::DescribePipeline(const FString& PipelinePath)
{
	UAFAuthoring* Authoring = GetAuthoringChecked();
	if (!Authoring)
	{
		return TArray<FAFStepView>();
	}

	FString Error;
	TArray<FAFStepView> Views = Authoring->DescribePipeline(PipelinePath, Error);

	if (!Error.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}

	return Views;
}

TArray<FAFDecision> UAFToolset::GetPendingDecisions(const FString& RunId)
{
	UAFExecutor* Executor = GEditor ? GEditor->GetEditorSubsystem<UAFExecutor>() : nullptr;
	return Executor ? Executor->GetPendingDecisions(RunId) : TArray<FAFDecision>();
}

bool UAFToolset::AnswerPipelineGate(
	const FString& RunId, FName StepId, const FString& ItemKey, bool bApprove, const FString& Note)
{
	UAFExecutor* Executor = GEditor ? GEditor->GetEditorSubsystem<UAFExecutor>() : nullptr;

	if (Executor == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("The pipeline executor is not available."));
		return false;
	}

	if (!Executor->AnswerGate(RunId, StepId, ItemKey, bApprove, Note))
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(
			TEXT("Nothing in '%s' is waiting on '%s'%s. Call Get Pending Decisions to see what is."),
			*RunId, *StepId.ToString(),
			ItemKey.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" for '%s'"), *ItemKey)));

		return false;
	}

	return true;
}

bool UAFToolset::ConnectPipelineSteps(
	const FString& PipelinePath, FName FromStep, FName FromChannel, FName ToStep, FName ToPin)
{
	UAFAuthoring* Authoring = GEditor ? GEditor->GetEditorSubsystem<UAFAuthoring>() : nullptr;

	if (Authoring == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Pipeline authoring is not available."));
		return false;
	}

	FString Error;
	if (!Authoring->Connect(PipelinePath, FromStep, FromChannel, ToStep, ToPin, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
		return false;
	}

	return true;
}

bool UAFToolset::DisconnectPipelineStep(const FString& PipelinePath, FName ToStep, FName ToPin)
{
	UAFAuthoring* Authoring = GEditor ? GEditor->GetEditorSubsystem<UAFAuthoring>() : nullptr;

	if (Authoring == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Pipeline authoring is not available."));
		return false;
	}

	FString Error;
	if (!Authoring->Disconnect(PipelinePath, ToStep, ToPin, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
		return false;
	}

	return true;
}

bool UAFToolset::ArrangePipeline(const FString& PipelinePath)
{
	UAFAuthoring* Authoring = GEditor ? GEditor->GetEditorSubsystem<UAFAuthoring>() : nullptr;

	if (Authoring == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Pipeline authoring is not available."));
		return false;
	}

	FString Error;
	if (!Authoring->Arrange(PipelinePath, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
		return false;
	}

	return true;
}

bool UAFToolset::RenamePipelineStep(const FString& PipelinePath, FName StepId, FName NewStepId)
{
	UAFAuthoring* Authoring = GEditor ? GEditor->GetEditorSubsystem<UAFAuthoring>() : nullptr;

	if (Authoring == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Pipeline authoring is not available."));
		return false;
	}

	FString Error;
	if (!Authoring->RenameStep(PipelinePath, StepId, NewStepId, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
		return false;
	}

	return true;
}

FAFPipelinePlan UAFToolset::PlanPipelineRun(const FString& PipelinePath, int32 MaxItems)
{
	UAFExecutor* Executor = GetExecutorChecked();
	const UAFPipeline* Pipeline = LoadPipeline(PipelinePath);

	return Executor && Pipeline ? Executor->PlanPipeline(Pipeline, MaxItems) : FAFPipelinePlan();
}

void UAFToolset::SetStepShape(const FString& PipelinePath, FName StepId, EAFShape Shape)
{
	EditStep(PipelinePath, StepId, [Shape](FAFStep& Step) { Step.Shape = Shape; });
}

void UAFToolset::SetPipelineOutput(const FString& PipelinePath, const FString& OutputPath)
{
	UAFAuthoring* Authoring = GetAuthoringChecked();
	if (!Authoring)
	{
		return;
	}

	FString Error;
	if (!Authoring->SetPipelineOutput(PipelinePath, OutputPath, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

void UAFToolset::SetPipelineDescription(const FString& PipelinePath, const FString& Description)
{
	UAFAuthoring* Authoring = GetAuthoringChecked();
	if (!Authoring)
	{
		return;
	}

	FString Error;
	if (!Authoring->SetPipelineDescription(PipelinePath, Description, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

void UAFToolset::ClearStepBinding(const FString& PipelinePath, FName StepId, FName Pin)
{
	UAFAuthoring* Authoring = GetAuthoringChecked();
	if (!Authoring)
	{
		return;
	}

	FString Error;
	if (!Authoring->ClearBinding(PipelinePath, StepId, Pin, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

TArray<FString> UAFToolset::ListNodeNames(const FString& Category)
{
	UAFNodeRegistry* Registry = GetRegistryChecked();

	TArray<FString> Names;
	if (!Registry)
	{
		return Names;
	}

	for (const FAFNodeType& Node : Registry->FindNodeTypes(Category, FString()))
	{
		Names.Add(FString::Printf(TEXT("%s  -  %s"), *Node.Category, *Node.FunctionPath));
	}

	Names.Sort();
	return Names;
}
