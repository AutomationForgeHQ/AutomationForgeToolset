#include "AFToolsetModule.h"

#include "AFToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

DEFINE_LOG_CATEGORY(LogAutomationForgeToolset);

void FAutomationForgeToolsetModule::StartupModule()
{
	if (!UToolsetRegistry::IsAvailable())
	{
		return;
	}

	if (UToolsetRegistry::IsToolsetClassRegistered(UAFToolset::StaticClass()))
	{
		bRegistered = true;
		return;
	}

	// Skills auto-discover. Toolsets do not - forget this line and the whole toolset is invisible,
	// with no warning anywhere, which reads exactly like the plugin being disabled.
	UToolsetRegistry::RegisterToolsetClass(UAFToolset::StaticClass());
	bRegistered = UToolsetRegistry::IsToolsetClassRegistered(UAFToolset::StaticClass());

	UE_LOG(LogAutomationForgeToolset, Log, TEXT("Automation Forge toolset %s."),
		bRegistered ? TEXT("registered") : TEXT("failed to register"));
}

void FAutomationForgeToolsetModule::ShutdownModule()
{
	if (bRegistered && UToolsetRegistry::IsAvailable())
	{
		UToolsetRegistry::UnregisterToolsetClass(UAFToolset::StaticClass());
		bRegistered = false;
	}
}

IMPLEMENT_MODULE(FAutomationForgeToolsetModule, AutomationForgeToolset)
