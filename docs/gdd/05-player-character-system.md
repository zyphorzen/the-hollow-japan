# Step 0.5 — Player & Character System

Version: 0.5.0 | Status: Draft — awaiting approval

## 1. Design baseline

The Hollow Wanderer is a customizable, classless character. Eight-direction movement, four-direction animation, 16×32 px base sprite, 16×16 px world tiles, real-time combat with tactical pause, initial level cap 50, host-authoritative simulation. Appearance has no stat impact. Custom origins provide equal total starting bonuses, not permanent class restrictions.

## 2. Character creation

Name 3–24 characters (Unicode-aware validation); 2 body variants, 6 skin tones, 8 hairstyles, 8 hair colors, 6 eye colors, 6 outfits as content targets. Facial details and voices are deferred. MVP uses one body, two hair styles and a small palette. Origin, appearance and allocated points are validated by authority before creation. Stable character UUID is independent of display name.

| Origin            | Bonus                  | Starting technique  |
| ----------------- | ---------------------- | ------------------- |
| Fallen Samurai    | STR +2, VIT +1         | Basic Swordsmanship |
| Wandering Shinobi | DEX +2, AGI +1         | Silent Step         |
| Shrine Disciple   | SPI +2, WIL +1         | Spirit Pulse        |
| Village Hunter    | DEX +2, VIT +1         | Focused Shot        |
| Merchant's Child  | WIL +2, DEX +1         | Appraisal           |
| Orphan Wanderer   | VIT +1, AGI +1, WIL +1 | Survival Instinct   |

Origin also changes introductory dialogue and starter equipment. All techniques remain learnable by any origin.

## 3. Primary attributes and derived stats

STR strength; DEX dexterity; VIT vitality; AGI agility; SPI spirit; WIL willpower. Starting value 5 each, origin bonus 3 total, free allocation 5, 3 points per level above 1, base attribute cap 60. Initial total 38; level 50 total 185 if all points spent. Attribute respec is deferred to Step 0.7.

| Derived stat          | Baseline formula                      |
| --------------------- | ------------------------------------- |
| Max HP                | 100 + VIT × 12                        |
| Max stamina           | 50 + AGI × 6                          |
| Max spirit energy     | 40 + SPI × 8                          |
| Physical attack       | 10 + STR × 2                          |
| Spiritual attack      | 10 + SPI × 2.5                        |
| Physical defense      | 5 + VIT × 1.5                         |
| Critical chance       | 5% + DEX × 0.4 percentage points      |
| Max posture           | 40 + WIL × 5                          |
| Corruption resistance | min(40%, WIL × 0.5 percentage points) |

Derived stat = (base + attribute contribution + sum(flat modifiers)) × (1 + sum(additive percentage modifiers)) × product(multipliers); apply stat-specific clamps last. Recalculate on dirty state, not every frame. Clamp current resources to new maxima; equipment swapping never grants free healing. Critical chance capped at 75% provisionally. Corruption resistance applies to new exposure only, not existing corruption. Damage formulas are deferred to Step 0.6.

## 4. Movement and controls

Coordinates: world pixels, +X right, +Y down. Normalize diagonal input; fixed 60 Hz simulation, render interpolation independent of tick. Desired movement in pixels/second: walk 64, run 96, sprint 144, sneak 40. Sprint costs 12 SP/s, regeneration starts 1 second after expenditure; proposed 10 SP/s while idle/walking, no regeneration while sprinting. Default normal movement is run. Terrain and statuses modify speed; collision resolves swept motion or substeps against solid geometry. Player foot anchor is collision origin; proposed collider 12×10 px, separate sprite and combat hitboxes. Use foot-based Y-sort for actors. Never derive collision from transparent sprite bounds. Input: WASD / left stick, Shift sprint, Ctrl sneak, E interact, Space dodge, LMB primary, RMB secondary, Q tactical pause, Tab inventory, F realm action; all remappable. Walk available through remappable toggle. Controller support via SDL gamepad abstraction. Camera follows interpolated position; never used as authoritative collision source.

## 5. Animation and rendering

Four cardinal directions, diagonals choose dominant movement axis, tie retains previous axis. Combat facing may use aim independently. Visual layers: body, hair, outfit, armor, weapon and effects; use shared frame anchors and palette variants. Base sprite 16×32, feet anchor (8,30); larger attack frames allowed. Idle 4 frames at 4 fps; walk 6 at 8 fps; run 6 at 12 fps; sprint 6 at 14 fps; attack 6 at 12 fps; dodge 5 at 15 fps; hit 3 at 10 fps; death 6 at 8 fps. Placeholder timings only: authoritative attack windows belong to combat simulation, not renderer frame callbacks. Nearest-neighbor sampling, integer scaling, stable atlas IDs. Animation events are cosmetic unless tied to simulation ticks.

## 6. Equipment

Slots: main hand, off hand, head, torso, legs, feet, accessory 1, accessory 2, spirit charm. Two-handed weapons reserve both hands; off-hand conflict handled atomically or reject if no inventory space. Cosmetic layers do not change stats. No class restrictions; optional technique/stat requirements. Item instances have stable ID, definition ID, modifiers and ownership. Equipment stat changes validated by authority; swaps disallowed during attack, dodge, stagger, downed or death. Durability remains undecided. Weight/encumbrance deferred, no mandatory weight system in MVP.

## 7. Character state model

Use orthogonal state machines to avoid combinatorial states: life (Alive, Downed, Dead, Respawning); locomotion (Idle, Walk, Run, Sprint, Sneak); action (None, Attack, Block, Dodge, Cast, Interact, Stagger, Revive, RealmTransition). Priority: life restriction > stagger > committed action > locomotion. Attack and dodge may restrict movement; transitions controlled by combat data. Tactical pause is session state, not character state. Downed only in co-op; 20 seconds downed and 3 seconds revive channel as approved baseline; death respawns at last safe shrine. Cancel actions on death, realm transition or disconnect according to authority. No simultaneous conflicting actions. Status effects (poison, slow, burn, buffs, corruption) are separate timed records; refresh/stack policy per definition.

## 8. ECS architecture

Use a lightweight custom ECS initially: stable persistent UUID separate from runtime generational entity handle, sparse component storage, systems query components, no SDL dependency in simulation. Composition rather than Player extends Character extends Entity hierarchy. Registry owns component lifetime; deferred structural changes at tick boundary. Components: Identity, Transform (realm, position), Movement, Collider, Attributes, DerivedStats, Resources, LifeState, ActionState, StatusEffects, Equipment, InventoryRef, Progression, Origin, Appearance, AnimationState, PlayerController or AIController, NetworkIdentity. Animation presentation can live outside authoritative world. Player and NPC share common components; player controller maps commands, AI generates equivalent commands.

Tick order: collect input/AI commands → validate → resolve action transitions → movement/collision → combat/interactions → resource/status updates → rewards/events → dirty stat recompute → persistence/network snapshot. Recompute dirty stats before any later action that reads them. Render reads immutable/interpolated snapshots; authoritative fixed tick owns state. Avoid raw pointers across structural changes and stale entity IDs.

Proposed directories: src/gameplay/character/{CharacterFactory,AttributeSystem,StatCalculator,MovementSystem,CharacterStateSystem,EquipmentSystem,StatusEffectSystem}; src/ecs/{Registry,Entity,ComponentStorage}; src/presentation/animation/{AnimationSystem,SpriteAssembler}; src/input/{InputMapper,PlayerCommand}. Headers and implementation split as required, no empty scaffolding until needed.

## 9. Persistence and multiplayer

Persist schema version, character UUID, name, origin, appearance, allocated points, level/XP, learned techniques, current resources, corruption, equipment instance IDs, inventory reference and last safe shrine. Store world-specific location/realm and quest flags per world, not global character position. Do not persist transient input, animation frame, velocity or attack queue. Save atomically through repository; load validates bounds, item ownership and schema, recalculates derived stats, clamps resources and recovers unsafe positions at shrine. Co-op host validates commands and owns authoritative movement, resources, equipment and progression rewards; clients predict local movement and reconcile from tick-stamped snapshots, interpolate remote players. Account trust/import policy is deferred to Step 0.10; offline characters cannot be assumed tamper-proof. Tactical pause follows unanimous voting online.

## 10. Acceptance criteria and scope

Prototype: one sprite, movement in 8 directions without diagonal speed advantage, collision at chunk boundaries, four-direction animation, fixed tick independent of FPS. Vertical slice: origins, allocation, stat recomputation, equipment, death/respawn, save/load, basic state transitions. Multiplayer alpha: command validation, prediction/reconciliation, host-authoritative equipment and resource updates. Tests: total allocation budget, stat bounds, equipment atomicity, stale handles, conflicting actions, pause, invalid input, save/load and cross-platform replay. No implementation or balancing test has been performed. All numeric values and custom ECS choice are proposed, pending approval and technical architecture review.

## Git checkpoint

```bash
git add docs/
git commit -m "docs(gdd): finalize gameplay loop and define player character system"
```
