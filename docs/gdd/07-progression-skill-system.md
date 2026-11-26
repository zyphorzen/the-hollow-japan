# Step 0.7 — Progression & Skill System

Version: 0.7.0 | Status: Draft — awaiting approval

## 1. Design baseline

Multi-path classless progression: character level, attributes, weapon mastery, learned skills, professions, reputation and world knowledge. Level 1–50, six attributes, 3 attribute points and 1 skill point per level above 1; weapon mastery 1–100 per family; profession 1–50. Host validates progression in co-op. All balancing values are provisional.

## 2. Character XP

`XP_next(L) = 100 + 50*(L-1) + 10*(L-1)^2` for levels 1–49. Cumulative XP is the sum of thresholds from 1 through L-1. Carry overflow across levels; at 50 redirect eligible overflow to Renown. Use integer arithmetic and checked 64-bit counters. Total XP to 50: 443940.

| Source          |             Base XP |
| --------------- | ------------------: |
| Minor enemy     |               15–35 |
| Standard enemy  |              40–100 |
| Elite           |             120–300 |
| World boss      |            500–3000 |
| Side quest      |             100–500 |
| Main quest      |            300–2000 |
| Discovery       |              25–150 |
| Puzzle          |              30–200 |
| Noncombat quest | Equivalent quest XP |

Combat multiplier by enemy level minus player level: <=-10: 0.10; -9..-5: 0.40; -4..-3: 0.75; -2..2: 1.00; 3..5: 1.20; >=6: 1.35. Floor once. Main quests, first discovery and unique rewards exempt. Grant once per eligible claim; noncombat resolutions receive comparable rewards. Repeatable encounters use unique occurrence IDs. Co-op grants individual eligible participants rewards based on contribution (damage, healing, defense, objectives), presence and realm; no last-hit requirement. Host validates eligibility and deduplicates grants.

## 3. Attributes

STR, DEX, VIT, AGI, SPI, WIL. Base 5 each; origin +3 total; 5 free creation points; 147 level-up points at level 50; total 185 including base and origin, maximum base 60 each. Unspent points retained. Effective attributes may exceed 60 through modifiers. Reuse Step 0.5 derived formulas; dirty-stat updates and resource clamping, no free heal on allocation. Validate point balance, cap and ownership atomically.

## 4. Weapon mastery

`MXP_next(M) = 40 + 12*(M-1) + 2*(M-1)^2` for 1–99. Separate mastery per weapon family (Katana, Wakizashi/Dual Blades, Odachi, Yari, Naginata, Kusarigama, Yumi, Spirit Staff, Ofuda, Unarmed). Eligible successful hits, effective defense, techniques and encounter completion grant mastery; cap per encounter; no empty swings, invulnerable targets or friendly fire farming. Origin techniques are free. Mastery unlocks eligibility, not automatic purchase of advanced skills. No large automatic per-level damage multiplier.

| Mastery | Rank        |
| ------- | ----------- |
| 1–19    | Novice      |
| 20–39   | Apprentice  |
| 40–59   | Adept       |
| 60–79   | Expert      |
| 80–99   | Master      |
| 100     | Grandmaster |

Katana examples: 1 Basic Swordsmanship; 10 Quick Draw; 20 Flowing Blade; 40 Moon Counter; 60 Crimson Crescent; 80 Silent Severance; 100 Thousand Petal Blade. Mastery rewards and per-encounter cap remain tuning parameters.

## 5. Skill tree and points

Six branches: Martial, Shadow, Sacred, Elemental, Hollow, Survival. DAG with stable node IDs, prerequisites, level, mastery, mentor/quest requirements and costs. Tier I/II/III/IV/V minimum level 1/10/20/35/45 and cost 1/2/3/4/5 skill points (SPt, distinct from stamina SP). Level 50 earns 49 SPt. Basic and origin-granted skills free, not refundable. Cross-branch investment allowed. No cycles, dangling references, or double purchase. Respec refunds purchased nodes only. Four active slots R/Z/X/V separate from consumables 1–4 and realm F; remappable. Change loadout outside combat only; pause cannot bypass restrictions. Cooldowns keyed by ability, not slot.

| Branch   | Example nodes and effects                                                                                                                                                                                                                                |
| -------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Martial  | Battle Focus (I, melee stamina -5%); Flowing Blade (II, combo); Iron Guard (II, block cost -10%); Moon Counter (III, parry follow-up); Breaker Strike (III, posture); Unyielding Resolve (IV, stagger resistance); Thousand Petal Blade (V, mastery 100) |
| Shadow   | Silent Step (I, noise); Keen Eye (I, detection); Quickstep (II, dodge cost); Phantom Strike (III, stealth attack); Shadow Veil (IV, concealment); Vanishing Moon (V, evasion)                                                                            |
| Survival | Iron Body (I); Deep Breath (II); Herbal Knowledge (II); Explorer's Instinct (III); Unshaken Mind (IV); Wilderness Mastery (V)                                                                                                                            |

Example prerequisite chains: Battle Focus -> Flowing Blade -> Moon Counter (mastery 40); Spirit Pulse -> Moonveil Barrier -> Purification; Spirit Pulse -> Hollow Slash -> Rift Burst -> Hollow Avatar. Free origin unlock satisfies prerequisite without refund. Additional mastery and quest gates apply.

## 6. Sacred Arts

| Skill              | Tier |  SE | Cooldown seconds |
| ------------------ | ---- | --: | ---------------: |
| Spirit Pulse       | I    |  12 |                3 |
| Healing Light      | II   |  18 |               10 |
| Moonveil Barrier   | II   |  20 |               12 |
| Purification       | III  |  25 |               20 |
| Sacred Sanctuary   | IV   |  35 |               30 |
| Celestial Blessing | V    |  50 |               90 |

Barrier absorbs 40 damage for 5s; Purification removes one cleansable debuff. Healing = floor((HealBase + 1.2*SPI)*healMultiplier), capped at missing HP; cannot revive dead players. Sanctuary area support; all skills subject to realm, range, target and action validation. Cooldowns use simulation ticks.

## 7. Elemental Arts

Fire: Flame Bolt, Crimson Inferno (damage over time). Water: Water Spear, Tidal Prison (control). Lightning: Thunder Strike, Storm Breaker (burst). Wind: Wind Cutter, Tempest Field (mobility/control). Water + lightning conductive bonus; fire + wet consumes Wet and weakens burn; water extinguishes burn; wind spreads burn to bounded targets; spiritual attacks may conditionally benefit against corruption. Reactions use internal cooldown, target cap and unique event IDs; no recursive infinite reactions. Prototype fire/lightning, full reactions post-vertical-slice. Exact coefficients deferred.

## 8. Hollow Arts

| Skill         | Tier |  SE | Base corruption percentage points |
| ------------- | ---- | --: | --------------------------------: |
| Hollow Sense  | I    |   0 |                                 0 |
| Hollow Slash  | II   |  18 |                                 5 |
| Void Step     | III  |  22 |                                 8 |
| Rift Burst    | III  |  35 |                                10 |
| Soul Devour   | IV   |  40 |                                15 |
| Hollow Avatar | V    |  50 |                                25 |

Hollow Slash cooldown 6s; Rift Burst 18s (Step 0.6). Other timings TBD. `gain = baseGain*(1-resistance)`, resistance = clamp(0.005\*WIL + passive modifiers, 0, 0.40). Fixed-point accumulation, clamp corruption 0–100; apply only on committed cast, not rejected command; purification independent. At 100 Fully Hollow is not automatic game over; story and social consequences apply. Avatar is temporary transformation distinct from persistent corruption state.

## 9. Passive skills

Iron Body HP +5%; Deep Breath stamina regen +10%; Spiritual Affinity SE +8%; Keen Precision crit +2 percentage points; Unshaken Mind posture +8%; Soul Resistance corruption resistance +5 percentage points (within 40% cap); Herbal Knowledge gathering efficiency +10%; Efficient Smith material discount; Explorer's Instinct discovery radius. Stable source IDs, stacking rules, stat-specific caps; no duplicate modifiers on reconnect. Proposed cooldown reduction cap 40%, resource cost reduction cap 50%; positive-cost abilities cost at least 1 unit. No resource generation from negative costs.

## 10. Professions

Blacksmithing, Alchemy, Cooking, Fishing, Farming, Gathering. Independent level 1–50; `PXP_next(P)=60+20*(P-1)+4*(P-1)^2` (1–49). Ranks: 1–9 Novice/basic; 10–19 Apprentice/improved; 20–29 Artisan/rare materials; 30–39 Expert/advanced; 40–49 Master/legendary eligibility; 50 Grandmaster/signature. Grant XP on valid completed activity, lower XP for trivial recipes, no character XP per routine craft; milestone/quest XP allowed. Recipe economics and quality deferred to 0.8.

## 11. Reputation

Independent integer score -100..100 for Crimson Shogunate, Moonveil Order, Shadow Lotus, Wandering Blades and Hollow Covenant. -100..-75 Hostile; -74..-25 Unfriendly; -24..24 Neutral; 25..49 Friendly; 50..74 Trusted; 75..100 Allied. Affects encounters, services, quests and techniques. Cross-faction consequences are explicit quest deltas, not automatically symmetric. NPC personal relationship independent. World-specific faction state remains with host world; irreversible story flags not removed by score changes. Previously learned skills remain learned unless explicitly flagged as faction-conditioned; access policy data-driven.

## 12. Respec

Ritual of Renewal at eligible shrine, outside combat/downed/realm transition. First free, later cost `100+50*(L-1)` currency. Refund only actually allocated free/level points and purchased skill points; preserve origin/base, mastery, profession, XP, reputation, corruption, quest decisions. Validate new build, unequip incompatible gear only if inventory/storage has capacity; otherwise reject transaction. Remove invalid loadout entries, preserve cooldown timers; atomic currency debit and state update. Host validates and persists first-free claim.

## 13. Build examples at level 50

| Build            | STR | DEX | VIT | AGI | SPI | WIL |
| ---------------- | --: | --: | --: | --: | --: | --: |
| Crimson Samurai  |  60 |  25 |  40 |  25 |  10 |  25 |
| Phantom Shinobi  |  25 |  60 |  25 |  55 |  10 |  10 |
| Moonveil Sage    |  10 |  15 |  30 |  20 |  60 |  50 |
| Hollow Reaper    |  45 |  15 |  25 |  20 |  55 |  25 |
| Spirit Guardian  |  25 |  15 |  60 |  15 |  25 |  45 |
| Wandering Hunter |  20 |  60 |  35 |  40 |  10 |  20 |

All total 185, base <=60; illustrative, not locked classes.

## 14. Endgame and balancing

Horizontal progression via mastery, rare equipment, spirit expeditions, world bosses, faction rewards, hidden quests and achievements. At character level 50 eligible XP redirects to Renown; 10000 XP per prestige rank, cap 100, cosmetic/title rewards only, no attributes/damage/skill points. Stop accumulating at prestige cap. No forced daily login. Level 5 target 30–60min; 10: 2–4h; 20: 8–15h; 35: 25–40h; 50: 50–80h. These are hypotheses; simulate encounter/quest XP rates and test pacing before release. Crit cap 75%, corruption resist cap 40%, four active slots. Preserve horizontal choices and counterplay from combat spec.

## 15. Architecture and persistence

Modules: ProgressionSystem, ExperienceSystem, AttributeAllocationSystem, WeaponMasterySystem, SkillTreeRegistry, SkillUnlockSystem, SkillLoadoutSystem, ProfessionSystem, ReputationSystem, RespecService, RewardLedger. Gameplay systems independent of SDL renderer. Data files: progression/level_curve.json, mastery.json, skills.json, professions.json, reputation.json, prestige.json. Stable IDs, schema/version validation, DAG cycle detection. Persistent character data: level, XP, allocated/unspent points, learned/free skills, mastery XP/levels, professions, loadout, renown, respec count; world-scoped reputation and quest eligibility; reward ledger keyed by world/player/claim. Persist transactionally; host checks command IDs, player ownership, eligibility, budgets and duplicates. Clients display snapshots and request actions, never grant XP. Save migration preserves spent points and skills or refunds removed nodes deterministically. Offline character import policy deferred to 0.10.

## 16. Milestones and acceptance criteria

Prototype: XP thresholds, level-up, allocation, Katana mastery and simple unlocks. Vertical slice: three weapons, basic skill tree, two professions, reputation and respec. Multiplayer alpha: authoritative grants, co-op eligibility, replay/reconnect protection and atomic saves. Full release: complete branches, six professions, mastery 100, prestige and endgame balancing.

Tests: threshold and overflow incl. level 50; point conservation; reject attribute >60; mastery independent; no invalid target rewards; DAG cycle and missing prereq rejection; free skill no refund; insufficient currency respec rolls back; equipment capacity rollback; no duplicate XP or skill purchase on reconnect; cooldown unchanged by loadout; fixed-point corruption and resistance cap; profession/reputation bounds; prestige cannot exceed cap or grant power; migration and Linux/Windows consistency. All implementation tests pending.

## Git checkpoint

```bash
git add docs/
git commit -m "docs(gdd): finalize combat and define classless progression"
```

Decisions: XP curves, skill costs, caps, profession ranks, respec price, prestige and pacing are proposed design values; no implementation, benchmark or playtesting yet. Specific coefficients, drop rates, advanced abilities and economy tuning remain open.
