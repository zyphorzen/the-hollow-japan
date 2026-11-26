# Step 0.9 — NPC & Quest System

Version: 0.9.0 | Status: Draft — awaiting approval

## 1. Vision and baseline

Living World, Meaningful Choices. NPCs use custom ECS, authored personality, schedule, FSM and utility scoring; dialogue and quests use versioned graphs. Relationships are character/world-specific; faction reputation world-specific. Main quests branch; dynamic quests use authored templates. Optional romance and human/spirit companions. SQLite persistence and host authority. No external AI required.

## 2. NPC architecture

NPC definitions store stable ID, name, origin, faction, personality, appearance, home, schedule, dialogue graph and service references. Runtime ECS handles have generation counters; persistent NPC ID survives unload. Optional components: Identity, Transform, Personality, Schedule, Dialogue, Relationship, AIController, QuestGiver, Merchant, Companion, LifeState. Categories: story, citizen, merchant, mentor, faction, traveling, companion, spirit, hostile and ambient. Important NPCs are handcrafted; ambient NPCs may be template-generated with stable seed-derived IDs. Personality values courage, compassion, honesty, greed, loyalty, curiosity range 0–100 and select authored behaviour, never generate canonical lore.

## 3. Aokawa roster and schedules

| NPC     | Role             | 06–08      | 08–16      | 16–19      | 19–22      |
| ------- | ---------------- | ---------- | ---------- | ---------- | ---------- |
| Haruto  | Elder/main quest | Shrine     | Square     | Hall       | Home       |
| Akari   | Sacred mentor    | Prayer     | Shrine     | Ritual     | Home       |
| Takeshi | Blacksmith       | Forge prep | Forge      | Forge      | Home       |
| Hana    | Alchemy mentor   | Gathering  | Shop       | Shop       | Home       |
| Ren     | Martial mentor   | Training   | Grounds    | Inn        | Inn        |
| Yuki    | Merchant         | Stock prep | Store      | Store      | Home       |
| Sora    | Fishing mentor   | Fishing    | River      | Market     | Home       |
| Kuro    | Spirit companion | Contextual | Contextual | Contextual | Contextual |

22–06: rest for human NPCs. Schedule overrides in priority order: death/incapacitation, immediate danger, critical story, quest obligation, weather/festival, daily schedule, idle. Navmesh/grid pathfinding uses bounded work; blocked route retries then safe authored fallback. NPC service availability depends on location and activity. Never teleport an observed NPC just to satisfy schedule. Full simulation near players, reduced decisions farther away, abstract schedule transitions in unloaded chunks, dormant persistent state. Quest-critical escort/combat stays simulated or suspends explicitly. Offscreen simulation uses world clock (no offline progression). NPC death permanent only where authored; essential roles protected or replaced via explicit alternate path.

## 4. Relationships

Affinity range -100..100 per character/NPC/world. Hostile -100..-75; distrust -74..-25; neutral -24..24; friendly 25..49; trusted 50..74; close 75..100. Personal affinity independent of faction reputation and story flags. Example gains: meaningful dialogue +2 once, relevant quest +10, preferred gift +5, betrayal -25. Gifts max one rewarded gift per NPC per in-game day; repeat dialogue cannot farm affinity. Trust unlocks dialogue and personal quests, not automatic romance. NPC reaction considers corruption, faction, quest history and affinity with explicit priority. No passive daily decay for MVP.

## 5. Dialogue

Graph nodes: line, choice, condition, effect, jump, end. Stable IDs, speaker, localization keys, next edges; conditional choices may be hidden or disabled with reason. Host validates current session/node/revision, distance, realm, life state, prerequisites and ownership of any payment. Effects are typed commands, never arbitrary script execution. Choice effects commit atomically with dialogue/quest state. Dialogue is personal and does not pause shared simulation; cutscenes use explicit group readiness. Cycles permitted only with explicit exit and bounded automatic traversal; validate missing references and unreachable mandatory nodes. Save committed decisions, not transient dialogue sessions.

Example Corrupted Shrine: Akari asks player to investigate; destroy spirit (+10 Shogunate, -5 Moonveil), purify (+10 Moonveil, shrine restored), absorb (+10 Shadow Lotus, +10 base corruption subject to resistance), leave (unresolved). Branch rewards equivalent in character XP for resolved branches, distinct items/flags. Choices require actual objective completion, not just selecting dialogue.

## 6. Quest architecture and lifecycle

Quest definition: ID, version, scope, type, prerequisites, objective graph, completion/failure conditions, reward bundles, repeat policy, localization and world effects. Quest instance: UUID, definition/version, owner scope, state, objective progress, accepted tick/day, branch choice, revision and claim IDs. States: Locked → Available → Active → ReadyToTurnIn → Completed; Active may transition to Failed or Abandoned if allowed. Reward claim and completion atomic; Completed is terminal for instance. Repeatable quests create new instance and cycle ID. Main story cannot be abandoned; optional quests may restart only per explicit policy. Objective states inactive/active/completed/failed/skipped. Graph supports AND/OR, sequential and mutually exclusive branch groups. Use event subscriptions rather than polling every quest each frame.

Objective types: TalkTo, ReachLocation, Defeat, Collect, Deliver, Craft, Interact, Discover, Escort, Protect, Survive, RealmTravel, Choice, ReputationThreshold. Count only relevant events with unique IDs; distinguish possessing items from acquiring after acceptance; delivery consumes items atomically. Timed objectives use simulation clock and explicit deadline; no expiration during pause or offline. Conditions support typed AND/OR/NOT, comparisons of level, mastery, item count, faction, affinity, flags, realm, time and corruption. Evaluate on host; no arbitrary eval. Explicit recovery/alternate giver for unavailable NPC or invalidated objective.

## 7. Quest catalog and story

Main chapters remain from Step 0.2: The Awakening; Whispers of the Dead; Land of Broken Shrines; War of the Clans; Beyond the Veil; The Forgotten Truth; Eclipse of the Realms; The Hollow Sovereign; A World Reborn. Shared story branch follows host decisions, ending eligibility from reputation, corruption and story flags. Side quests: Missing Fisherman (Sora, rescue/investigate); Forge of Resolve (Takeshi, craft); Petals of Memory (Hana, gather/lore); A Blade Without a Master (Ren, duel or dialogue); The Corrupted Shrine (Akari, branching). Faction quests: Crimson Shogunate defend patrol; Moonveil restore shrine; Shadow Lotus investigate forbidden relic; Wandering Blades escort merchant; Hollow Covenant investigate rift. Rewards and cross-faction effects authored, not automatically symmetric.

Dynamic quests use seeded templates: bounty, delivery, gathering, rescue, defense, investigation. Select eligible region, targets, reachable locations, threat level, reward budget and expiry. Generate stable instance IDs on host. Initial cap 3 offers per region, refresh daily, active offers persist until completion/expiry. Do not target dead essential NPCs, inaccessible zones or exhausted unique resources. No mandatory daily login. Unique quests never regenerated as dynamic quests.

## 8. Romance and companion

Optional adult NPC romance; eligible adult player and NPC, affinity >=75, personal quest completed, mutual consent through explicit dialogue, compatible story flags. Stages: acquaintance, friendship, trust, mutual interest, partnership. Rejection respected; gifts do not guarantee romance. One active committed romance initially; breakups via explicit dialogue, no combat penalties. Romance not MVP and no exclusive power bonuses. NPCs with unsuitable roles or story conditions are not romanceable.

Companions: human or spirit; one active companion per player, shared party cap four companions subject to profiling; vertical slice only Kuro with exploration support. Modes Follow, Stay, Assist, Retreat. Host owns AI, position, combat, inventory. Companion pathfinding avoids blocking player; leash and safe relocation out of combat only. Downed companions recover at shrine; authored story exceptions. Kuro detects nearby spirit clues and supports realm traversal, not unrestricted teleportation. Ren candidate human combat companion; Akari candidate support after personal quest. Companion loyalty independent of romance. No companion inventory in MVP; later use same container transaction rules.

## 9. Persistent consequences

World flags are typed, scoped, versioned and applied through atomic effect bundles. Example restored shrine unlocks fast travel and reduces local instability; destroyed spirit removes encounter and changes dialogue; absorbed spirit opens Hollow path and increases corruption; ignored shrine stays unresolved. Effects use unique event IDs; do not replay on loading. Separate permanent flags from reversible states. Main quest outcomes have explicit post-game variants and fallback routes. Essential quest graph tested for reachability across all supported endings; no accidental softlocks.

## 10. Multiplayer quest synchronization

Host owns shared story, world flags, NPC schedules and objective validation. Personal quests/affinity stored by character within host world; shared objectives counted once per unique event, participant eligibility captured at event time. Shared story decisions: host final choice after optional advisory vote; personal choices belong to interacting player unless they affect shared world (host approval). Shared story progress does not overwrite guest's own world. Guest receives eligible individual rewards, personal progression; unique claims scoped by world/quest instance/player and reward policy. Late join receives snapshot + revision, not automatic historical rewards. Disconnect suspends personal session; host can continue shared quests. Reconnect reconciles snapshot and deduplicates events. Conflicting NPC interactions serialized; exclusive quest choices use optimistic revision and atomic commit. Network stays active during unanimous tactical pause.

## 11. Technical design

Modules: gameplay/npc/{NpcRegistry,ScheduleSystem,NpcAISystem,RelationshipSystem,DialogueSystem,CompanionSystem}; gameplay/quest/{QuestRegistry,QuestSystem,ObjectiveSystem,ConditionEvaluator,QuestEventRouter,WorldEffectService,QuestRewardService,DynamicQuestGenerator}. Data directories assets/data/{npcs,schedules,dialogues,quests,quest_templates,companions}; localization keys separate. SQLite tables npc_states, npc_relationships, dialogue_flags, quest_instances, quest_objectives, quest_events, world_flags, reward_claims, command_ledger. Shared transaction service from Step 0.8 ensures inventory/currency/XP/flags/claim atomicity; publish events only after commit. UI reads snapshots, sends commands.

```json
{
  "id": "aokawa_corrupted_shrine",
  "version": 1,
  "scope": "world",
  "type": "side",
  "prerequisites": { "all": [{ "flag": "met_akari", "equals": true }] },
  "objectives": [
    { "id": "reach", "type": "ReachLocation", "target": "forgotten_shrine" },
    {
      "id": "resolve",
      "type": "Choice",
      "requires": ["reach"],
      "options": ["destroy", "purify", "absorb"]
    }
  ],
  "repeatable": false,
  "reward_bundle": "aokawa_shrine_resolution"
}
```

Example only: resolution options also require validated gameplay events; schema and IDs subject to implementation.

## 12. Milestones and acceptance

Prototype: one NPC, schedule, interaction, linear dialogue, one quest, save/load. Vertical slice: Aokawa roster, conditional dialogue, branching shrine quest, 3 side quests, Kuro support, relationship and world flags. Multiplayer alpha: shared/personal scope, revision, late join, reconnect, atomic claims. Expansion: dynamic quests, factions, romance, human companions, full story.

- [ ] NPC schedule and overrides deterministic across chunk unload/load.
- [ ] Dialogue validates choices and cannot repeat one-time rewards.
- [ ] Quest graph validates references, branches and reachability.
- [ ] Objective events deduplicate and preserve progress after save/load.
- [ ] Delivery/rewards/world effects commit atomically.
- [ ] Main quests have fallback for unavailable NPCs.
- [ ] Relationships and faction reputation remain separate.
- [ ] Dynamic quests never target unreachable or invalid objectives.
- [ ] Companion AI respects realm, collision and authority.
- [ ] Romance remains optional and consent-based.
- [ ] Multiplayer late join/reconnect cannot duplicate rewards.
- [ ] Shared decisions do not overwrite guest world state.

All values, names of new quests and architecture are proposed design, not implemented or playtested. Git checkpoint: `git commit -m "docs(gdd): finalize economy and define npc quest system"`.
