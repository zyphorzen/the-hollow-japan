# Step 0.4 — Core Gameplay Loop

Version: 0.4.0 | Status: Finalized — approved

## 1. Philosophy and loop

Exploration-driven RPG with consequential progression. Design principles: freedom, risk/reward, meaningful progression, consequences. Explore → encounter → overcome (combat, dialogue, puzzle, stealth or retreat) → acquire reward → improve → prepare → explore. Rewards unlock opportunities rather than forcing repetitive combat.

| Loop | Target duration | Activity |
|---|---|---|
| Micro | 5–30 sec | Movement, attack, dodge, interaction |
| Encounter | 1–5 min | Combat, puzzle, gathering |
| Adventure | 10–30 min | Quest, dungeon, discovery |
| Session | 30–90 min | Upgrade, reputation, progression |
| Long-term | Multiple sessions | Story and world changes |
| Endgame | Ongoing | Mastery, challenges, exploration |

## 2. Exploration

Points of interest: ancient shrines, abandoned villages, caves, bandit camps, sacred trees, spirit rifts, ruins, secret gardens, world bosses, traveling merchants. Rewards include lore, materials, techniques, equipment and faction opportunities. Map states: Unexplored, Discovered, Visible; discovered areas persist per player and realm. Custom markers, environmental clues, journal and shrine fast travel. Retreat remains possible; no mandatory level gate for ordinary regional exploration.

## 3. Combat loop

Observe → engage → react → adapt → resolve → recover. Resources: HP (health), SP (stamina), SE (spirit energy), PO (posture), HC (Hollow Corruption). Attack, dodge, parry, block, stealth and retreat are options; exact rules in Step 0.6. Tactical pause freezes gameplay offline; online requires unanimous consent, with host authority and no unilateral pause. Queued actions execute after resume subject to validation, costs and timing.

## 4. Realm and corruption loop

Discover rift → prepare → enter Kakureyo → investigate or fight → retrieve spiritual reward → return → cleanse or embrace corruption. Realm transitions require a valid destination and cannot bypass collision or combat restrictions. Corruption is persistent and influences power, NPC reactions and story routes; it is not a mandatory loss condition. Preview thresholds: 0–19 stable, 20–39 touched, 40–59 unstable, 60–79 corrupted, 80–99 near Hollow, 100 fully Hollow. Cleansing at shrines, quests or consumables; transformation route remains playable. Exact thresholds and modifiers are provisional.

## 5. Survival and recovery

Light survival, not a survival simulator. Hunger and thirst are not mandatory meters. Food gives temporary buffs, resting restores resources, weather and extreme temperatures matter in relevant regions. Stamina regenerates outside demanding actions; health recovery uses rest, healing items or skills. Safe rest at villages, houses or unlocked shrines. No automatic starvation death. In combat, recovery items obey use time and cooldown. Rest does not force multiplayer time skip.

## 6. Gathering and crafting

Locate node → gather with appropriate tool → receive materials → process at station → craft/upgrade → use or trade. Categories: plants, wood, ore, fish, spirit materials. Nodes have stable IDs, depleted state and respawn policy tied to world time. Host validates proximity, tool, capacity and atomic reward. Crafting requires known recipe, materials and station; no negative inventory or duplication. Initial recipes: healing salve, basic blade, spirit charm. Later professions: farming, cooking, alchemy, blacksmithing and fishing. Durability, if added, must not dominate play; deferred to Step 0.8.

## 7. Progression and rewards

Classless progression via character XP, weapon mastery, learned techniques, equipment, discoveries and reputation. XP granted for combat, quests, discoveries and noncombat solutions; first-discovery rewards granted once. No forced grinding or permanent class lock. Reward types: currency, materials, equipment, techniques, knowledge, access and story consequences. Unique rewards use stable claim IDs. Level ranges are guidance, not blanket enemy scaling. Exact XP curves, stats and skill trees in Steps 0.5 and 0.7.

## 8. Quest and social loop

Discover request → investigate → choose approach → resolve → receive consequences → unlock opportunities. Main quests, side quests, bounties, faction quests, personal NPC quests, world events. Host owns shared story decisions; individual quest rewards and relationship data scoped explicitly. Journal records objective, hints, state and choices. Reputation can unlock or close opportunities; no critical-path softlocks without alternate route.

## 9. Death and failure

Offline: defeat → respawn at last attuned safe shrine; retain equipment, XP, quest progress and essential items. Proposed penalty: lose 10% of carried ordinary currency into one recoverable cache; new defeat replaces prior cache. Recovery claim is atomic, single-use; inaccessible cache placed at reachable fallback. No loss of premium/quest items; no paid revival. Co-op: downed state for 20 seconds, ally revive with interruptible 3-second channel; if expired or party wiped, respawn at safe shrine. No duplicate reward or cache creation on reconnect. Boss encounters reset according to encounter policy. All numbers are provisional.

## 10. Co-op loop

Host creates world → friends join → explore together or separately → resolve encounters → distribute individual loot → persist host world and personal progression. Enemy scaling by participating players (not all connected players); initial trial HP multipliers 1.0/1.6/2.1/2.5 for 1–4 participants, locked per encounter. Shared decisions belong to host; personal progression and loot remain individual. Rejoin must not duplicate claims. Global tactical pause requires unanimous consent; timeout/disconnect cancels pending vote, and host can resume. Pause policy and transport finalized in Step 0.10.

## 11. Endgame

Post-story world reflects ending. Optional mastery challenges, world bosses, procedural spirit expeditions, faction restoration, rare recipes and secrets. No daily login obligation or mandatory repetitive grind. New Game Plus is optional post-release scope. Endings must preserve a playable post-game state, with alternate opportunities where content closes.

## 12. Aokawa vertical slice

Wake at Forgotten Shrine → meet Akari → gather herbs → craft healing salve → encounter Minor Spirit → learn tactical pause → discover rift → enter limited Spirit Realm → defeat or resolve corrupted spirit encounter → return to village → claim reward → unlock upgrade and next exploration objective. Target 20–30 min first playthrough, subject to testing. Prototype scope: movement, collision, interact, 1 resource, 1 recipe, 1 enemy, 1 quest, 1 shrine, 1 realm transition, reward and save/load. Housing, farming, full factions, seasons and multiplayer deferred until foundations work.

## 13. Implementation contracts

Simulation independent of SDL renderer. Inputs become commands; host/local authority validates commands; systems update at fixed tick; events drive UI, audio, quest and reward processing. Persistent IDs for world, player, node, encounter and reward. Separate per-player and shared world state. Offline and co-op share gameplay rules; network transport is an adapter. Proposed modules: GameplaySession, InteractionSystem, CombatSystem, ResourceSystem, CraftingSystem, ProgressionSystem, QuestSystem, RewardSystem, RealmSystem, RespawnSystem. Do not create all modules before required by a milestone.

## 14. Acceptance criteria

- Player can finish the Aokawa loop and begin another adventure.
- Combat, retreat and noncombat resolutions are distinguishable and rewardable where intended.
- Resource depletion, quest decisions and discovered map survive save/load.
- Rewards and crafting transactions cannot grant duplicates or create negative inventory.
- Death retains essential progression; recovery cache claimed at most once.
- Realm transition checks valid destination and preserves independent realm state.
- Offline pause freezes simulation, online pause needs unanimous approval; no queued action bypasses validation.
- Co-op host validates actions, personal loot is isolated, reconnect is idempotent.
- Performance target 60 FPS remains a target until benchmarked.

## 15. Decisions and open questions

Step 0.3 approved as design baseline; technical parameters remain subject to benchmarking. Step 0.4 is proposed, not implemented. Survival rules, death penalty, timings, scaling, recipe names and reward policy need approval/playtesting. Exact combat, skill, economy, multiplayer and architecture contracts are finalized in later GDD steps. No code, tests or benchmarks were run for this design step.

## Git checkpoint

```bash
git add docs/
git commit -m "docs(gdd): finalize world building and define core gameplay loop"
```
