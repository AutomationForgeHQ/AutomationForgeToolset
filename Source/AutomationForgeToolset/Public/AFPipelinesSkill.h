// What an agent needs to know about pipelines that the tool signatures cannot say.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/AgentSkill.h"
#include "AFPipelinesSkill.generated.h"

/**
 * How generation pipelines are shaped, and what makes one expensive to get wrong.
 *
 * Deliberately free of tool and property names, which rot. What belongs here is the model and the
 * failure modes - the things no signature can express.
 */
UCLASS()
class AUTOMATIONFORGETOOLSET_API UAFPipelinesSkill : public UAgentSkill
{
	GENERATED_BODY()

public:

	UAFPipelinesSkill()
	{
		Description = TEXT(
			"Build or change a generation pipeline: reading source assets, generating voice, faces or "
			"motion across many items, and writing the results back. Load this when asked to voice a "
			"whole project rather than one line, to run a pass overnight, to find out what a batch "
			"would cost before spending it, or when a re-run has generated things that already "
			"existed.");

		Instructions = TEXT(
			"A pipeline is data, not a graph. It is an ordered list of steps over a keyed hierarchy of "
			"items, where every step ADDS CHANNELS AND NEVER REWRITES THEM. That one property is what "
			"makes the whole thing inspectable at any point, and why a failure on item 23 leaves the "
			"other 39 holding everything they had.\n"
			"\n"
			"THREE SHAPES, AND THE WHOLE EXECUTOR IS THESE. **Expand** turns one item into many at a "
			"deeper level - reading a project into dialogues, a dialogue into its lines. **Map** adds "
			"channels to an item: generating, solving, ingesting. **Reduce** folds every item at a "
			"level back into its parent, which is what writing forty results into one asset is. A step "
			"declares which level it works at, and items at other levels are carried through untouched.\n"
			"\n"
			"Values flow two ways through that hierarchy and both are load-bearing. They BROADCAST "
			"DOWN, so something recorded for a line is readable by every alternative reading of it - "
			"that is how one gesture serves three takes without being copied onto each. And they GROUP "
			"UP, so a reduce sees its children as a collection.\n"
			"\n"
			"THE KEY IS THE MOST CONSEQUENTIAL THING IN A DEFINITION, and it is where the expensive "
			"mistake lives. An item's key must be built from STABLE NATURAL IDS in the source - a "
			"dialogue's own node ids, which its compiler already guarantees unique - and never from a "
			"display name, a file path, an array index, or anything containing the content itself. "
			"Build a key out of a line's text and rewriting one word makes it a different item: the "
			"ledger has never seen it, nothing is current, and the run pays again for work that "
			"already exists. Nothing errors. The only symptom is the bill.\n"
			"\n"
			"COST BEFORE COMMIT, ALWAYS. Check a definition first: it resolves every node, proves every "
			"input reads a channel something upstream actually produces, and spends nothing. Then plan "
			"the run, which reports how many items are already current, how many would be generated, "
			"and what that would cost. Present that number to the user before running anything metered. "
			"A node that has declared no cost is reported as unknown rather than counted as free - if a "
			"plan says the cost is incomplete, say so rather than quoting the total.\n"
			"\n"
			"WHAT MAKES SOMETHING CURRENT is a hash of everything that decided it: which node, its "
			"version, the provider, the provider's MODEL VERSION, and the resolved inputs. The model "
			"belongs in there and it is the part people leave out - without it, switching a generator "
			"serves the old result from the cache and a project ends up holding output from two models "
			"with no way to tell them apart.\n"
			"\n"
			"HASHABLE IS NOT THE SAME AS REPRODUCIBLE, and conflating them is a silent bug. A provider "
			"with no seed - or one whose seed is only best-effort, which is what ElevenLabs says about "
			"itself - produces different output from identical inputs. Such a result is a cache entry "
			"and an archive: it can be recognised, it can never be recreated, and it must never be "
			"deleted on the assumption it could be.\n"
			"\n"
			"A HUMAN'S EDIT OUTRANKS THE PIPELINE. A slot somebody has hand-edited is marked graduated: "
			"it reads as current whatever the inputs now hash to, and a pipeline writing to it is "
			"refused rather than allowed to overwrite. The history of what produced it is kept, because "
			"that history is the brief for whoever makes the next one.\n"
			"\n"
			"LONG WORK STARTS, IT DOES NOT FINISH. A face solve is minutes and a speech batch longer, "
			"so those nodes return once the work is under way and a companion node reports on it. A run is "
			"therefore not a call: starting one hands back an id straight away and the run advances itself, "
			"so nothing blocks and a run outlives the editor session that began it. Never wait on one in a "
			"loop of your own.\n"
			"\n"
			"A STEP ON SUCH A NODE WAITS, AND WHAT FINISHED LOOKS LIKE IS PART OF THE DEFINITION. The node "
			"usually declares it - a solve is finished when it reports it is no longer running - and a step "
			"inherits that, so most steps need not say anything. Where a step must, it names the field on "
			"the report and the value that means done. Wrong in either direction the failure is quiet: a "
			"step that waits for nothing runs the next one on top of work still in flight, and a step "
			"watching a field that does not exist waits out its whole timeout. Both are refused before a "
			"run starts rather than discovered during one. There IS a timeout and it fails rather than "
			"being patient - if it fires, the work may still be running on the provider, so check before "
			"starting it again, because a second start can pay twice.\n"
			"\n"
			"The good status nodes read their answer from the ASSET rather than from memory, which is what "
			"lets a pass interrupted by a restart be picked up instead of repeated - and is the reason a "
			"human decision is never a prompt but a suspended run with a record attached. A node that has "
			"only started work records nothing in the ledger yet: caching a result that does not exist "
			"would make the next run skip the item and find nothing there.\n"
			"\n"
			"THE NODE LIBRARY IS CURATED, NOT EVERYTHING CALLABLE. A function is a node only when "
			"somebody has said so and declared what it costs, whether repeating it is safe, and what "
			"level it works at - and only a person can answer those. If a capability is missing from "
			"the palette it has not been annotated yet; adding the marker is the fix, and guessing at a "
			"cost on somebody else's metered API is not. Nodes are found by reflection across whatever "
			"is installed, so the palette covers other people's plugins on the same terms as ours.\n"
			"\n"
			"Finally, the order to work in, because it is not the obvious one: name the steps and the "
			"key first, check the definition, plan the cost, show it, and only then run. Building the "
			"steps and discovering the key afterwards is how a pipeline ends up correct and expensive.");
	}
};
