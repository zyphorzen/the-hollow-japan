# Step 0.6 — Hybrid Combat System

Version: 0.6.0 | Status: Draft — awaiting approval

## 1. Vision and baseline

Real-time directional action combat with optional soft lock-on and tactical pause. Fixed simulation at 60 Hz; rendering independent. Combat emphasizes readable telegraphs, timing, positioning and resource management rather than raw level. Classless weapon mastery; host authoritative in co-op. Friendly fire disabled. All numbers below are provisional balancing targets.

## 2. Weapon families

| Weapon | Role | Hand slots | Main scaling |
|---|---|---|---|
| Katana | Balanced slash, counter | One or two by item | STR/DEX |
| Wakizashi / dual blades | Fast close range | One each | DEX/AGI |
| Odachi | Heavy stagger | Two | STR |
| Yari | Long thrust | Two | STR/DEX |
| Naginata | Sweeping crowd control | Two | STR/DEX |
| Kusarigama | Technical control | Two | DEX |
| Yumi | Ranged, ammo | Two | DEX |
| Spirit staff | Spiritual area attacks | Two | SPI |
| Ofuda | Support, barriers | One | SPI/WIL |
| Unarmed | Close-range martial arts | Empty hands | STR/AGI |

Vertical slice: Katana, Yumi, Spirit Staff; remaining families deferred. Each weapon definition contains damage channels, scaling, reach, moveset, stamina, posture, projectile and animation references. Requirements are attributes or learned techniques, never classes. Main-hand primary and off-hand secondary map to action definitions; two-handed weapons occupy both slots.

## 3. Damage resolution

For physical: AP = 10 + 2*STR; spiritual: AP = 10 + 2.5*SPI, using derived stats from Step 0.5 including valid modifiers. A ranged weapon may instead use AP = 10 + 2*DEX (proposed weapon-specific basis). For each channel: raw = (weaponBase + AP*scaling)*motionMultiplier*criticalMultiplier*otherModifiers. Do not apply the same equipment bonus twice. Physical/spiritual mitigation = raw*100/(100+max(0,defense)); spiritual defense baseline = 5+1.5*WIL. Elemental mitigation = raw*(1-clamp(resistance,-0.5,0.8)); elemental channel specifies its own AP source or zero. Crit chance from Step 0.5 (5%+0.4%*DEX), cap 50%; crit multiplier 1.5, eligible direct attacks only; roll once per attack-target pair on host. Sum positive mitigated channels then floor once, minimum 1 only for a non-immune damaging hit; full immunity, perfect parry and valid invulnerability give zero. Block modifies remaining HP damage before rounding. No friendly hits. Clamp HP to [0,maxHP]. Example STR15, weapon30, scaling0.8, motion1, DEF25: raw62, final49; heavy1.6 gives79. Modifiers must be finite/nonnegative; damage order and rounding tested.

## 4. Attack timing and combos

| Katana action | Startup ticks | Active ticks | Recovery ticks | SP | Motion | Posture damage |
|---|---:|---:|---:|---:|---:|---:|
| Light 1 | 8 | 4 | 12 | 8 | 1.0 | 12 |
| Light 2 | 9 | 4 | 13 | 10 | 1.1 | 14 |
| Light 3 | 12 | 5 | 18 | 14 | 1.4 | 22 |
| Heavy | 18 | 6 | 24 | 20 | 1.6 | 30 |
| Charged | 30 | 6 | 30 | 28 | 2.2 | 45 |

One tick = 1/60 s. Startup telegraphs attack, active resolves hits, recovery limits action. Charged starts on hold, releases after threshold; cost paid at commit; interrupted attacks do not refund. Light combo queues at most one next input during last 8 recovery ticks, executes after recovery if valid; expires on stagger, death, realm transition or timeout. Dodge cancel allowed only last 6 recovery ticks of Light 1/2; no unrestricted cancel. Heavy/charged cannot be canceled after commitment except forced interruption. Facing locked during active phase. Per attack instance target hit set prevents repeated damage across active frames; multi-hit techniques define explicit sub-hit IDs. Movement during attacks uses action-defined multiplier; knockback swept against collision. No animation frame determines gameplay damage.

## 5. Defense and posture

Dodge: 18 ticks, cost 18 SP, 48 px total travel, i-frames ticks 3–10 inclusive, cooldown 12 ticks after completion. Cannot phase through walls or realm boundaries; environmental hazards bypass dodge when explicitly tagged. Block: hold secondary action on compatible gear, frontal 120-degree arc (±60°), 70% HP reduction for physical blockable damage; spiritual block depends on equipment, default 30%. Block consumes 6 SP plus 0.25*pre-block mitigated damage, rounded up; insufficient SP causes guard break and hit resolves unblocked. Parry: distinct secondary tap/timing action, startup 3 ticks, window 5 ticks, recovery 18 ticks, cost 12 SP; compatible weapon only; parryable frontal hit in window negated, attacker loses 25 posture and may stagger. Failed parry has recovery. Unblockable attacks show distinct telegraph, cannot be blocked/parried; dodge if tagged avoidable.

Posture uses remaining capacity, max = 40+5*WIL. Hits reduce posture by attack-defined value; block may still reduce posture. At zero, stagger 60 ticks (boss data may override), no attack/dodge; restore 50% capacity when stagger ends. Recovery 10 posture/s after 2 s without posture damage, only when not staggered. Guard break also triggers stagger. Never allow infinite stagger loops: boss-specific resistance and recovery immunity. Stamina max = 50+6*AGI; sprint 12/s, regen 10/s while idle/walking after 1 s without spending; no regen during attack, dodge, sprint or block. Insufficient stamina rejects action without spending or starting cooldown. Health does not regenerate in combat by default. Spirit max = 40+8*SPI; initial regen 2/s outside combat after 3 s, plus shrine/consumables. Corruption resistance = min(40%,0.5%*WIL), applied to positive exposure only.

## 6. Spiritual arts and corruption

| Art | SE | Cooldown | Effect | Base corruption |
|---|---:|---:|---|---:|
| Spirit Pulse | 12 | 3 s | Short spiritual burst | 0 |
| Moonveil Barrier | 20 | 12 s | Absorbs 40 HP damage, 5 s | 0 |
| Purification | 25 | 20 s | Cleanse one eligible debuff | 0 |
| Hollow Slash | 18 | 6 s | Spiritual melee arc, motion 1.8 | 5 |
| Rift Burst | 35 | 18 s | Area spiritual damage, motion 2.2 | 10 |

SE consumed and cooldown begins at commit; targeting/range/realm validated. Barrier consumed after mitigation before HP; no stacking, refresh replaces remaining strength. Corruption clamp 0–100; threshold states from Step 0.4; purification at shrines follows gameplay loop. 100 opens Fully Hollow path, not automatic death. Skills cannot bypass costs through pause. Status examples burn (periodic HP), poison (periodic HP), slow (speed modifier), stagger (action state), spiritual mark (damage modifier). Same-source status refreshes duration by default, distinct sources obey per-effect stack cap. Tick-based durations pause with simulation. Damage over time is noncritical by default.

## 7. Tactical pause

Offline Q freezes gameplay tick, world clock, AI, projectiles, resource regeneration and cooldowns; UI and network transport remain responsive. Queue up to 3 planned commands per player; target, action, direction, optional item. On resume execute sequentially at legal action boundaries, revalidate current costs, range, alive/realm/state/cooldown; invalid command rejected, later valid commands may proceed. No instant item use while paused. Cancel/edit queue while paused; manual input cancels outstanding plans by default. Online unanimous vote of connected active players (10 s wall-clock timeout); host broadcasts pause state at tick boundary; any active player can request resume; disconnect cancels pending vote or resumes paused session to avoid deadlock. No pause in unsupported competitive modes. Host enforces rate limit, proposed 15 s wall-clock cooldown between successful pauses. Pause control uses network/wall time, not simulation time.

## 8. Hit detection and simulation

Environment collider 12x10 px at feet, hurtbox and attack hitbox independent from sprite. Example katana reach 24 px from anchor, 100-degree arc; broadphase spatial grid/chunk index then narrowphase circle/sector/AABB overlap. Swept segment/capsule for fast projectiles; terrain occlusion and realm filters. Hit only once per attack-target-subhit ID. Stable iteration order and unique entity handles; resolve by tick and stable IDs, define simultaneous trades by collecting valid hit candidates from same tick before applying damage. Priorities: life/state restrictions, realm/team, terrain/range, invulnerability, parry, block, damage channels, barrier, HP/posture, status, events/death. Define parry/posture interactions consistently for simultaneous candidates; host authoritative. Debug overlay shows colliders, hurtboxes, hitboxes and current tick. Attack definition stores startup/active/recovery in integer ticks, hit shape and offset, costs, tags, scaling, cancel windows, animation key. SDL presentation reads events, never writes combat result.

## 9. Enemy AI and boss design

FSM: Idle/Patrol -> Investigate -> Chase -> Telegraph -> Attack -> Recover -> Chase; alternative Retreat, Stagger, Dead. Perception: distance, line of sight, sound, same realm; memory timeout 5 s; initial detection 112 px, disengage 192 px, configurable by archetype. Utility scoring chooses attack using range, cooldown, stamina, target threat, ally proximity and deterministic tie-break. Navigation uses A* over chunk navigation, repath throttled; no attacks through walls. AI decision tick 10 Hz baseline, committed timing 60 Hz. Leash prevents dragging encounters indefinitely; reset restores enemy health only after disengage, no duplicate reward. Archetypes: Minor Spirit (basic melee), Wild Boar (charge), Bandit (block/feint), Tengu (ranged/flight), Oni (heavy posture), Wraith (realm mechanics). Offscreen AI throttled subject to active combat needs.

Aokawa boss The Hollow Stag: phase 1 100–>70% HP: charge and antler sweep; phase 2 <=70%: spirit projectiles and marked ground zones; phase 3 <=30%: Hollow surge, shorter recovery and rifts. Phase transitions monotonic, no repeated rewards; telegraph before active hit, safe escape route, readable visual/audio cues. Threshold checks once per tick after damage; if a hit crosses multiple thresholds enter highest applicable phase at next legal transition; death wins over phase transition. Adds capped at 3, cleared on reset. Boss posture stagger shorter (30 ticks), 3 s stagger immunity after recovery. Rewards first kill once per world/player claim policy; repeat encounters separate reward table. No unavoidable damage as default. Encounter resets on party wipe, not on individual death.

## 10. Multiplayer combat

Host validates sequenced input, ownership, current state, cooldown, resources, target/range/realm and hit geometry; clients never submit authoritative damage or rewards. Host 60 Hz simulation, 20 Hz snapshots (provisional), reliable ordered messages for critical events and deduplicated action/reward IDs; movement snapshots can be unreliable sequenced. Clients predict movement and local presentation; host confirms damage, corrects invalid predictions. No arbitrary client timestamps; optional bounded rewind for ranged hit checks later, not MVP. Boss HP multipliers 1/1.6/2.1/2.5 for 1–4 participating players, locked on encounter start; participants defined by presence in arena and realm when encounter starts, late join policy blocks arena entry or uses existing scaling until next encounter. No automatic enemy damage multiplier initially. Downed 20 s, revive channel 3 s, restores 30% HP, interrupted by damage/movement; host validates proximity (24 px) and same realm. Party wipe resets boss; shrine respawn and 10% currency recovery follow Step 0.4. Individual loot with idempotent claim IDs; disconnect cannot duplicate items. Offline character import trust policy deferred to Step 0.10.

## 11. Architecture and data

Proposed systems: CombatCommandSystem, ActionSystem, HitDetectionSystem, DamageSystem, DefenseSystem, PostureSystem, StatusEffectSystem, AbilitySystem, ProjectileSystem, EnemyAISystem, EncounterSystem, CombatEventSystem. Components: Combatant, ActionState, Resource, Posture, Hurtbox, Weapon, AbilityCooldown, StatusEffect, Projectile, Threat, BossPhase. Reuse Step 0.5 ECS and world collision; no direct SDL dependency in simulation. Data files assets/data/combat/{weapons,attacks,abilities,status_effects,enemies,bosses}.json. Validate schema and references at load. Stable attack instance ID and tick, immutable definition data; runtime mutable state separate. Tick order: validate commands -> update actions -> movement/collision -> collect hit candidates -> resolve -> apply HP/posture/status/death -> rewards/events -> snapshot. Stat recomputation before consumers, deterministic event ordering. No code implementation yet.

## 12. Milestones and acceptance criteria

Prototype: katana light, one enemy, hurtbox/hitbox, damage, stamina, dodge, debug overlay. Vertical slice: three weapons, combo, block/parry/posture, two spiritual arts, tactical pause, three enemy types, Hollow Stag, reward and respawn. Multiplayer alpha: authoritative actions, snapshots, vote pause, downed/revive, locked scaling, disconnect and idempotency. Full release: remaining weapons/arts, boss roster, advanced AI and balancing.

Tests: same attack across active ticks damages once; 60 Hz timing independent of rendering; zero SP rejects commands; diagonal movement and dodge wall collision; immunity yields zero; block/parry arc and exact window boundaries; posture break/recovery; cooldown/status stop on pause; queued invalid commands rejected; no cross-realm/friendly damage; projectile sweep; boss transitions monotonic; wipe reset; duplicate packet/reward claim idempotency; host rejects forged damage; Linux/Windows replay checks; profiling at 60 FPS target. All criteria pending implementation/playtesting.

## Git checkpoint

```bash
git add docs/
git commit -m "docs(gdd): finalize character system and define hybrid combat"
```

All combat constants, weapon roster, formulas, boss phases and AI policy are proposed; no benchmark or playtesting performed. Technical networking library and detailed skill tree remain deferred.
