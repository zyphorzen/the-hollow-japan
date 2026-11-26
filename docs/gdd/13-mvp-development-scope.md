# Step 0.13 — MVP & Development Scope

Version: 0.13.0 | Status: Draft — awaiting approval

## 1. Strategy and scope authority

Vertical Slice First, Incremental Development. This chapter governs delivery scope; earlier approved chapters describe the full game, not mandatory first-release features. Offline singleplayer MVP first, LAN multiplayer alpha separately, online private co-op thereafter. Preserve shared authoritative simulation, local transport, world-scoped characters, C++23/SDL3 and Linux/Windows targets from Steps 0.10–0.12. Full game retains eight regions, three realms, level 50 and 2–4 player co-op. No full engine, competitive PvP, public matchmaking or real-money economy in MVP. No implementation or benchmark is claimed.

## 2. MVP and acceptance scope

| Feature         | MVP                                                | Full game                  |
| --------------- | -------------------------------------------------- | -------------------------- |
| Regions         | Aokawa only                                        | Eight                      |
| World           | 256×256 tiles                                      | 4096×4096 tiles target     |
| Realms          | Mortal and limited Spirit                          | Three                      |
| Character level | 1–10                                               | 1–50                       |
| Weapons         | Katana, Yumi, Spirit Staff                         | Ten types                  |
| Professions     | Gathering, Blacksmithing                           | Six                        |
| NPC             | Seven humans + Kuro (conditional active companion) | Expanded                   |
| Boss            | Hollow Stag                                        | Multiple                   |
| Story           | Prologue + Chapter I segment                       | All chapters               |
| Playtime        | 45–90 minutes target                               | 50–80 h progression target |
| Multiplayer     | Separate alpha                                     | Online co-op 2–4           |

Must: engine, pixel-perfect camera, movement/collision, chunk streaming, basic character creation, combat including dodge/block/parry/tactical pause, inventory/equipment, XP/mastery, gathering/crafting, NPC/dialogue/branching quest, realm traversal, boss, SQLite save/load, keyboard and basic controller, accessible HUD, essential SFX, Linux and Windows package. Healing Salve can use a basic recipe station without full Alchemy profession progression. Should: lighting, limited weather, faction reputation, Kuro exploration assistance, repair/upgrades, advanced controller navigation, one procedural dungeon. Could: cosmetics, dynamic quest, fishing, elemental reactions, enemy variants. Deferred: full regions, romance, marriage, housing, farming, faction war, prestige, NG+, public matchmaking, relay infrastructure, PvP. Kuro NPC identity and realm traversal remain mandatory; companion follow AI conditional. If procedural dungeon is deferred, use handcrafted cavern or omit it; no mandatory quest depends on it. Basic crafting remains mandatory even without full profession trees.

## 3. Aokawa vertical slice

256×256 tiles, 16 px per tile, 4096×4096 px; 32×32 tile chunks, 8×8 chunks per realm (64 possible, limited spirit area); active radius 1, preload 2, unload beyond 3. Protected authored landmarks: Aokawa Village, Forgotten Shrine, bamboo forest, riverbank, spirit rift, Hollow Stag arena, optional Ruined Cavern. Region macro layout authored; seeded terrain detail cannot overwrite protected POIs. Quest path reachable in all supported seeds. Map layout provisional.

Content budget: 7 human NPCs (Haruto, Akari, Takeshi, Hana, Ren, Yuki, Sora), Kuro; 3 weapon types/6 definitions; 8 armor/accessories, 6 consumables, 12 materials, 8 recipes (some gated or optional), 3–5 enemies, 1 boss with three phases, 12 learnable nodes, 1 main quest chain, 4 side quests (including Corrupted Shrine), 3 mutually exclusive shrine resolutions, 6 music cues, 30–40 SFX target. No full profession progression for optional recipes. Content identifiers stable.

Adventure: Awakening at shrine → movement and interaction tutorial → village and Akari → gather herbs/craft salve → basic combat and tactical pause → limited Kakureyo traversal → resolve Corrupted Shrine (destroy/purify/absorb) → Hollow Stag → return, reward, world consequence, save. All three paths must leave a viable main quest route. First 20–30 minutes tutorial portion, entire MVP 45–90 minutes. Boss encounter accessible regardless of valid shrine choice; alternative safe shrine remains available. Do not require repetitive grinding to complete the adventure.

## 4. Roadmap and milestone gates

| Phase | Work                     | Exit gate                                                   |
| ----- | ------------------------ | ----------------------------------------------------------- |
| 0     | Pre-production           | M0 approved GDD v1.0.0                                      |
| 1     | Foundation               | M1 reproducible CMake build, window, logs, tests, CI        |
| 2     | Core engine              | Fixed ticks, ECS, commands, input and local transport tests |
| 3     | Rendering                | M2 camera, sprite, movement presentation                    |
| 4     | World/physics            | M3 explorable deterministic chunks, collision, clock        |
| 5     | Character/combat         | M4 encounter with attacks, defenses, AI                     |
| 6     | Inventory/progression    | M5 complete RPG loop, atomic crafting/loot                  |
| 7     | NPC/quest/realm          | Branching quest, limited realm travel                       |
| 8     | Persistence/presentation | M6 integrated vertical slice, save/load, UI/audio           |
| 9     | MVP QA                   | M7 packaged stable singleplayer MVP                         |
| 10    | Multiplayer alpha        | M8 LAN 2–4, authoritative state, reconnect                  |
| 11    | Online infrastructure    | M9 online co-op beta with tested connectivity               |
| 12    | Expansion/release        | M10 full game                                               |

Persistence contracts, command validation, test harness and stable IDs start early; DB-01/02 before economy mutations; network dependency smoke test early, full network gameplay later. No empty module scaffolding. No hard deadline before measured velocity. Estimates active engineering hours: foundation 20–40, core 50–100, rendering 60–120, world 80–160, combat 100–200, RPG 80–160, NPC 90–180, persistence/presentation 100–220, QA 80–160. Total 660–1340 hours before contingency; 30% buffer example 858–1742 hours; excludes full online and full original art/music production. Re-estimate after M1/M2.

## 5. Prioritized backlog

P0 = mandatory for milestone, P1 = important/conditional, P2 = optional. IDs stable. Each item includes tests and documentation.

| ID      | Task                        | Priority | Dependency                               |
| ------- | --------------------------- | -------- | ---------------------------------------- |
| FND-01  | Repository                  | P0       | M0                                       |
| FND-02  | CMake/Ninja/C++23           | P0       | FND-01                                   |
| FND-03  | Pin vcpkg tool and baseline | P0       | FND-02                                   |
| FND-04  | SDL3 window                 | P0       | FND-03                                   |
| FND-05  | Logging/errors              | P0       | FND-03                                   |
| FND-06  | Catch2/CI                   | P0       | FND-03                                   |
| CORE-01 | Math/time/IDs               | P0       | FND-06                                   |
| CORE-02 | Fixed tick                  | P0       | CORE-01                                  |
| CORE-03 | Sparse-set ECS              | P0       | CORE-01                                  |
| CORE-04 | Input mapper                | P0       | FND-04                                   |
| CORE-05 | Commands/events             | P0       | CORE-03                                  |
| CORE-06 | Local transport             | P0       | CORE-05                                  |
| RND-01  | Renderer abstraction        | P0       | FND-04                                   |
| RND-02  | Texture manager             | P0       | RND-01                                   |
| RND-03  | Camera                      | P0       | RND-01                                   |
| RND-04  | Sprite animation            | P0       | RND-02                                   |
| RND-05  | Atlas                       | P1       | RND-04                                   |
| WRL-01  | Tile registry/map           | P0       | RND-03                                   |
| WRL-02  | Tile collision              | P0       | WRL-01                                   |
| WRL-03  | Movement                    | P0       | WRL-02, CORE-04                          |
| WRL-04  | Chunk lifecycle             | P0       | WRL-01                                   |
| WRL-05  | Deterministic generation    | P0       | WRL-04                                   |
| WRL-06  | Clock/realm IDs             | P0       | WRL-04                                   |
| WRL-07  | Basic navigation and jobs   | P0       | WRL-02, CORE-03                          |
| CMB-01  | Attributes                  | P0       | CORE-03                                  |
| CMB-02  | Attack states               | P0       | CMB-01                                   |
| CMB-03  | Hitbox/damage               | P0       | CMB-02, WRL-02                           |
| CMB-04  | Dodge/block/parry           | P0       | CMB-03                                   |
| CMB-05  | Stamina/posture             | P0       | CMB-04                                   |
| CMB-06  | Enemy AI                    | P0       | CMB-03, WRL-07                           |
| CMB-07  | Tactical pause              | P0       | CORE-02, CMB-02                          |
| CMB-08  | Spiritual arts              | P0       | CMB-05                                   |
| RPG-01  | Item registry/inventory     | P0       | CORE-03                                  |
| RPG-02  | Equipment/loot              | P0       | RPG-01, CMB-03, DB-02                    |
| RPG-03  | XP/mastery/skills           | P0       | CMB-01, CORE-05                          |
| RPG-04  | Gathering/crafting          | P0       | RPG-01, WRL-04, DB-02                    |
| RPG-05  | Wallet/merchant             | P0       | RPG-01, DB-02                            |
| RPG-06  | Upgrade/repair              | P1       | RPG-02, RPG-05                           |
| NPC-01  | Schedule                    | P0       | WRL-06                                   |
| NPC-02  | Dialogue                    | P0       | NPC-01                                   |
| NPC-03  | Quest objectives            | P0       | CORE-05, NPC-02                          |
| NPC-04  | World consequences          | P0       | NPC-03, DB-02                            |
| NPC-05  | Realm traversal             | P0       | WRL-04, NPC-03                           |
| NPC-06  | Kuro follow AI              | P1       | NPC-05                                   |
| DB-01   | SQLite/migrations           | P0       | FND-03                                   |
| DB-02   | Atomic mutation             | P0       | DB-01, CORE-05                           |
| DB-03   | Save/load                   | P0       | DB-02, WRL-04                            |
| DB-04   | Reward ledger/recovery      | P0       | DB-02                                    |
| DB-05   | Backup/migration tests      | P0       | DB-03                                    |
| UI-01   | HUD/menu                    | P0       | RND-01, CMB-01                           |
| UI-02   | Inventory/dialogue UI       | P0       | RPG-01, NPC-02                           |
| UI-03   | Audio/settings              | P0       | FND-04                                   |
| UI-04   | Lighting/weather            | P1       | RND-01, WRL-06                           |
| MVP-01  | Aokawa integration          | P0       | WRL-05, NPC-05, RPG-04, RPG-03           |
| MVP-02  | Hollow Stag                 | P0       | CMB-06, CMB-08                           |
| MVP-03  | End-to-end QA               | P0       | MVP-01/02, DB-04/05, UI-01/02/03, CMB-07 |
| MVP-04  | Packages                    | P0       | MVP-03                                   |

Basic navigation is mandatory because enemy AI requires it; advanced asynchronous navigation can be deferred. All P0 backlog items must pass release gates, even if not a direct dependency of the final package.

Multiplayer: NET-01 GNS smoke test (FND-03); NET-02 codec (CORE-05); NET-03 authenticated LAN handshake (01/02); NET-04 two-player movement (03/WRL-03); NET-05 prediction/reconciliation (04); NET-06 replication/chunks (05/WRL-04); NET-07 combat (06/CMB-08); NET-08 guest profiles (DB-03/03); NET-09 quests/economy (07/08/DB-04/NPC-04); NET-10 trade/reconnect (09); NET-11 four-player tests (10); NET-12 private lobby/auth (03); NET-13 NAT/relay feasibility (12); NET-14 dedicated packaging (11); NET-15 online QA (13/14). M8 requires M7 or stable equivalent gameplay baseline. M9 requires M8.

## 6. Risk register

| Risk                         | Likelihood/impact | Mitigation / trigger                                                                          |
| ---------------------------- | ----------------- | --------------------------------------------------------------------------------------------- |
| Scope creep                  | High/high         | Freeze P0; trade out equal effort; no expansion before M7                                     |
| Custom engine complexity     | High/high         | Thin vertical increments; no generic engine/editor; adopt proven libs where justified         |
| Dependency incompatibility   | Medium/high       | Linux/Windows smoke builds, pinned revisions; no speculative version numbers                  |
| Multiplayer complexity       | High/high         | Local command path early; LAN first; delay infrastructure; no rollback initially              |
| Asset production bottleneck  | High/high         | Placeholder art, limited roster, licensed packs or original assets; separate asset estimates  |
| Poor performance             | Medium/high       | Profile p95 on target laptop; cap active entities, lights and particles; reduce effects first |
| Save corruption/duplication  | Medium/high       | SQLite transaction ledger, crash injection, backups, migration tests                          |
| Procedural inaccessible POIs | Medium/high       | Authored protected masks, path checks, reproducible seeds                                     |
| Branching quest softlock     | Medium/high       | Test each resolution, fallback NPC/routes                                                     |
| Solo developer burnout       | High/high         | Small milestones, realistic hours, WIP limit 1–2, contingency                                 |
| Disk space/build caches      | Medium/medium     | Check free disk before vcpkg; keep build outside small root partition when possible           |
| License violations           | Medium/high       | Asset manifest, license audit before packaging                                                |
| NAT/CGNAT                    | High/high         | Direct/LAN baseline, relay feasibility spike, dedicated fallback                              |

Scope change protocol: document request, affected chapter and backlog IDs, cost/risk, trade-off, acceptance tests; obtain approval before changing baseline. Blocker > two focused sessions triggers investigation/spike and ADR, not arbitrary feature expansion.

## 7. Performance targets and measurement

Target device i5-8250U/UHD 620/12 GB; Linux and Windows x86-64. 480×270 world target, 60 FPS and 60 Hz; frame p95 ≤16.67 ms target, server tick p95 ≤12 ms; vertical slice process RSS <512 MiB target; texture budget 96 MiB, decoded audio 32 MiB, 512 particles, 16 lights, 32 audio voices. Network snapshots 20 Hz, normal upload ≤10 KiB/s and download ≤50 KiB/s per client, RTT target ≤200 ms. Measure release builds after warmup with recorded hardware/settings/seed/build hash; repeat village, forest, boss, realm transitions, streaming and four-player scenarios; report p50/p95/p99, peak RSS and errors, not only average FPS. Headless soak 2h for multiplayer. These are targets, not results. If misses occur, profile and reduce presentation cost/active entities before changing simulation rate; any relaxed target requires approval.

## 8. Definition of done and release gates

Task DoD: accepted behavior, focused implementation, tests incl. boundary/failure paths, no new sanitizer failures, format/static checks, documentation updated, no secrets, reproducible build, reviewed diff, meaningful Git commit. Feature DoD additionally requires integrated gameplay, save/load where applicable, authoritative validation and asset licensing. Milestone DoD: all P0 tasks accepted, no open blockers, test logs and build artifacts recorded; optional items explicitly deferred.

M7 release: fresh installs on Linux and Windows; complete adventure across three shrine resolutions; save/load/restart and backup recovery; inventory full and crash tests; combat timing independent of FPS; no critical/high severity defects, softlocks, data loss or known client duplication; basic input/settings/audio and readable telegraphs; asset notices included; performance report and accepted deviations; packages and SHA256 checksums. M8 adds 2–4 LAN players, independent guest progression, 0–5% loss scenarios, reconnect, trade/quest consistency, no known replay duplication, two-hour soak. M9 adds verified identity, private lobby, NAT cases, relay or documented fallback, abuse limits, external connectivity and dedicated package. Full release adds complete storyline/regions/endgame, content QA, distribution policies. No release solely because roadmap date arrived.

## 9. Step-by-step implementation start

Phase 1 only after Step 0.13 approval. 1.1 inspect toolchain, disk, repository and Git state; no destructive cleanup. 1.2 create minimal CMake project and presets; compile a console smoke test. 1.3 bootstrap pinned vcpkg and install only SDL3 initially; test CMake discovery. 1.4 SDL3 window/event loop with clean shutdown. 1.5 add spdlog/error handling. 1.6 add Catch2/CTest. 1.7 Linux/Windows CI and build documentation. 1.8 foundation gate, tag engine milestone only after tests. Each step: explain goal, give commands and small code changes, run local checks, review user output, fix failures, Git checkpoint before next step. Do not install all planned dependencies immediately. Use separate build directories, avoid committing binaries/caches/secrets. Proposed commits: build: initialize C++23 project; build: configure pinned vcpkg dependencies; feat(platform): create SDL3 application window; feat(core): add logging; test: configure Catch2 and CTest; ci: add cross-platform build matrix.

## 10. GDD v1.0.0 finalization checklist

- Step 0.12 approved; Step 0.13 awaiting user approval.
- Verify earlier chapters remain intact and statuses consistent.
- Scope rules override earlier aspirational delivery quantities, not full-game vision.
- Review contradictions: basic navigation P0; persistence early; Kuro companion/procedural dungeon conditional; level cap 10 MVP only; world-scoped profiles; online infrastructure separate.
- Record provisional balancing, performance and effort estimates; exact dependency versions remain pending smoke tests.
- Approve GDD v1.0.0, update index/README, commit docs, optionally tag `gdd-v1.0.0` (not executable release).
- Begin Phase 1.1 with actual environment inspection. No implementation is completed by documentation approval.

## 11. Git checkpoint

```bash
git status
git add README.md docs/
git diff --cached --stat
git commit -m "docs(gdd): finalize technical architecture and define mvp roadmap"
```

Push only to the intended current branch after review. Final approval checkpoint later: `docs(gdd): approve pre-production baseline v1.0.0` and `git tag -a gdd-v1.0.0 -m "Approved GDD baseline"`.
