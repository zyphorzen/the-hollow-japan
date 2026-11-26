# Step 0.12 — Technical Architecture

Version: 0.12.0 | Status: Draft — awaiting approval

## 1. Engineering baseline

Custom modular 2D engine; C++23, SDL3, CMake >= 3.28, Ninja, Linux/Windows x86-64. Singleplayer, listen server and dedicated server use the same authoritative simulation. No full game engine, rigid-body engine, custom crypto or custom codec. 60 Hz simulation, 20 Hz network snapshots, 480x270 world rendering, 60 FPS target. All numbers are design targets, not benchmarks.

## 2. Dependencies and reproducibility

SDL3 (platform/render/input/audio), SDL3_image (PNG), SDL3_ttf (fonts), SDL3_mixer (mixing/decoding), nlohmann/json (content), SQLite (persistence), spdlog (logging), FastNoiseLite (terrain detail), GameNetworkingSockets (transport), Catch2 (tests), optional Tracy (profiling). vcpkg manifest with pinned builtin-baseline and pinned tool revision; verify actual port names, features, licenses and ABI on both platforms before freezing versions. GNS crypto/Protobuf and ICE feature choices need build tests. Do not follow floating main/master. Dependency versions remain deliberately unassigned pending smoke tests. No SDL dependencies on headless targets. Use CMake target usage requirements rather than global include paths.

## 3. Modules and dependency direction

`thj_core`: math, IDs, ECS, utility; `thj_contracts`: commands, events, immutable DTOs (depends core); `thj_world`: generation, chunks, navigation (core/contracts); `thj_simulation`: gameplay and authoritative state (world/core/contracts); `thj_protocol`: explicit binary codecs (contracts/core); `thj_network`: local and GNS adapters (protocol/contracts); `thj_persistence`: SQLite repository adapters (contracts); `thj_presentation`: SDL, UI, audio, assets (contracts/core); `thj_application`: orchestration and ports (simulation, network, persistence); `thj_client`: application + presentation; `thj_server`: application without SDL presentation; `thj_tests`: test targets. No cyclic dependencies; simulation declares ports and receives adapters through application composition. No socket, SDL texture, or database handle in gameplay components.

## 4. Repository blueprint

```
CMakeLists.txt
CMakePresets.json
vcpkg.json
.clang-format
.clang-tidy
cmake/{Dependencies,CompilerWarnings,Sanitizers}.cmake
src/{core,ecs,contracts,application,simulation,world,gameplay,network,persistence,presentation,input,assets,platform,client,server}/
assets/{source,runtime,data,manifests,licenses}/
tests/{unit,integration,simulation,networking,regression,fuzz}/
tools/{assets,scripts}/
benchmarks/
docs/{GDD.md,gdd/,adr/}
.github/workflows/
```

Gameplay subdirectories follow Steps 0.5–0.9. Presentation includes rendering, animation, particles, lighting, audio and UI. Network includes transport, protocol, session, replication, prediction and security. Create modules only when needed; no empty scaffolding explosion.

## 5. ECS

Custom sparse-set storage: 64-bit runtime handle (32-bit index + generation), sparse index, dense entity/component arrays, swap-and-pop removal, validity checks. Persistent UUID and replicated network ID are separate. Structural mutations deferred to explicit barriers. Never retain references across storage mutation. Stable ordering for order-sensitive systems, not unordered-map iteration. ECS single-writer initially; no parallel scheduler until profiling justifies it.

## 6. Game loop and deterministic simulation

Fixed 60 Hz, monotonic steady clock, 64-bit tick, integer/rational accumulator where needed. Client frame clamp 250 ms and max five catch-up ticks per frame; reset step count every frame. Dedicated server retains debt, reports overload and eventually rejects load or stops cleanly rather than silently discarding authoritative ticks. Offline may discard excessive wall-clock backlog with logging, not advance gameplay time invisibly. Pause freezes gameplay and resets timing debt; UI/network/wall-clock timeouts remain active. Input edges consumed once; held input can repeat. Snapshot interpolation is presentation only.

Tick sequence: drain commands; validate; structural barrier; update attributes/state; AI (10 Hz decisions); movement/collision; combat/projectiles; resource/status; interaction/economy; quest/progression; ordered domain events; persistence requests; immutable snapshot. Recompute dirty stats before consumers. Stable ordering, seeded independent RNG streams and canonical integer generation for cross-platform checksums. No claim of complete cross-platform determinism for arbitrary floating-point operations; server authoritative. 32-bit wire ticks use modular comparison with bounded windows.

## 7. Memory and resource ownership

RAII, unique_ptr for exclusive ownership, shared immutable snapshots only where justified, span for bounded non-owning views, vectors for dense storage, generational handles, pools for particles/projectiles, bounded ring buffers, PMR scratch arenas only after measurement. Never retain scratch references beyond tick/frame. No owning raw pointers or direct new/delete in gameplay. Application/world/session/frame lifetimes explicit. Limit queue sizes and decoded asset allocation before reading untrusted data.

## 8. Renderer

SDL3 Renderer first; no Vulkan/OpenGL custom backend unless measured need. Render extraction from snapshot -> visibility culling -> command buffer -> layer + foot-Y sorting -> compatible batching -> SDL renderer. Ground, decoration, objects/entities, overhead, light composite, emissive/particles, upscale, UI. Nearest filter, integer scale with optional fit mode, high-DPI UI independent. Texture atlas 2048 max initial, 2px extrusion, stable pivots. No reordering translucent sprites to optimize batches. Main thread owns SDL window/renderer/textures. Debug overlay displays collision, chunk boundaries, draw calls, frame/tick timings.

## 9. Physics and collision

Custom top-down kinematic system, no gravity. Tile AABB static collision, dynamic entity colliders, uniform spatial grid broad phase, AABB/circle/capsule/sector narrow phase, swept movement/projectile tests, sliding and corner cases, trigger overlaps. Separate sprite, foot collider (12x10 initial), hurtbox, hitbox. Filter by realm and collision masks: WorldStatic, Player, NPC, Enemy, Projectile, Hurtbox, Hitbox, Trigger. A\* navigation uses bounded incremental requests; paths invalidated by collision revision. Internal fixed-point with enough fractional bits (e.g. Q16.16 in int64, not signed int32 for full world range); wire positions 1/16 px int32. Use remainder accumulation to preserve speed at 60 Hz. Prevent collision across unloaded boundaries until ready. Tests for corners, diagonal normalization, negative chunk coordinates and tunneling.

## 10. Assets

Stable logical IDs resolve through manifest to atlas region, audio or definition. Lifecycle Unloaded -> Queued -> Loading -> CPUReady -> GPUReady -> InUse -> EvictionCandidate -> Unloaded; failure state and cancellation generation. Worker file read/decode; main thread GPU upload/destruction. Reference-counted leases, bounded LRU eviction of unreferenced assets; fallback for noncritical art, fail startup for missing required gameplay definitions. Preload UI/player/short SFX; region-based tilesets; encounter boss prefetch; stream music. Validate paths, hashes, dimensions, license IDs, references and schema before loading. Offline asset export/packing scripts deterministic; source assets separate from runtime export.

## 11. Input and audio

SDL events -> device state -> rebindable actions -> context -> command or UI navigation. Gameplay, dialogue, inventory, tactical pause, menu, debug contexts; analog dead zone, clamp magnitude, normalize diagonal digital movement, consume press edge once across catch-up ticks. Bindings per user in platform settings, not game state. Controller hotplug and focus-loss neutral input.

Audio: SDL3 Audio + SDL3_mixer, 48 kHz, WAV short SFX, Ogg Vorbis music; mono positional, stereo ambient/music; 32 voices initial. Master/music/ambience/SFX/UI/voice buses, distance attenuation, panning, priority, loop and crossfade. Callback cannot block or mutate gameplay. Presentation consumes event IDs, dedupes predicted vs confirmed cues. Headless server no audio device.

## 12. Event architecture

Typed in-process domain events, not stringly typed global singleton. Separate commands (intent), events (facts), queries (read-only), presentation events. Event queue scoped to world; ordered dispatch at barriers; subscriber lifetime explicit. Event ID includes world/session context and monotonic event counter or durable UUID as appropriate. Events cannot recursively mutate systems without a defined command/barrier; cap queue and fail loudly for authoritative overflow. UI/audio may drop low-priority cosmetic effects, never authoritative reward events. Critical effects + reward claims in a single durable transaction; use outbox records where delivery is retried. Example: EnemyDefeated -> QuestObjectiveAdvanced -> RewardGranted -> presentation cues, not direct UI calls.

## 13. Job system and threading

Start single-thread simulation + main SDL thread in same client loop if adequate; introduce separate authoritative simulation thread only after stable interfaces and profiling. Dedicated server uses simulation loop without renderer. Network adapter receives/sends on owned thread or pumps callbacks per library rules. Worker pool initial 2 workers on development laptop, configurable; std::jthread + stop_token, bounded queues, condition variables, no busy wait. Jobs: chunk generation, disk IO, pathfinding against immutable data, CPU asset decode. Job result contains request ID, world epoch and revision; stale result discarded. Main/simulation owner applies results at barriers. No worker mutates ECS or SDL textures. Single SQLite writer owns connection; readers separate connections when needed. Graceful shutdown: stop accepting commands, drain/commit critical writes, cancel noncritical jobs, join workers, release resources. Test cancellation, saturation and deadlocks.

## 14. SQLite and transaction contract

One authoritative local database per world, not shared over network filesystem. Tables: worlds, characters, character_world_state, item_instances, containers, inventory_slots, wallets, resource_nodes, chunks, npc_states, quest_instances, quest_objectives, world_flags, reward_claims, command_ledger, economy_events, outbox, schema_migrations. Foreign keys enabled per connection, prepared statements and parameter binding, UNIQUE/CHECK constraints, checked integer currency, WAL and synchronous=FULL for durability, bounded busy timeout. Only a single writer processes ordered transactions. Critical command: validate and reserve affected state on simulation owner -> immutable mutation plan + idempotency key -> writer transaction (validate DB revisions, mutate data, ledger/outbox) -> committed result -> apply to simulation at barrier -> acknowledge. No final success before durable commit; affected resources cannot be mutated concurrently while pending. On failure release reservation and reject; on crash after commit recover from DB, replay stored result by command ID. No periodic checkpoint overwrites newer committed revisions. SQLite FULL durability subject to storage/OS guarantees; verify current patched SQLite at dependency freeze.

## 15. Save system

```
user_data/
  settings.json
  worlds/<world_uuid>/world.db
  worlds/<world_uuid>/backups/
  logs/
```

Resolve OS user-data path through platform abstraction (SDL_GetPrefPath client or configured server path). DB authoritative metadata includes save_schema_version, generator_version, content_hash, seed, world clock, world UUID. Runtime entity IDs not persisted; use stable IDs. Critical economy/quest/reward transactions immediately durable; noncritical world/position checkpoint every 30 seconds; final save on orderly quit. Backup via SQLite Online Backup API, not raw copy of live DB alone; atomic publish completed backup; initial retention 3 rotating backups. Migration: version check, backup, ordered transactional migrations, integrity/foreign-key checks, refuse newer unsupported saves, restore only on explicit recovery. Seed + deltas; incompatible generator versions require old generator or migration, never silently regenerate modified worlds. Crash may lose noncritical changes since last checkpoint, not acknowledged critical mutations.

## 16. Networking and serialization

Step 0.10 remains canonical: GNS reliable/unreliable UDP; local transport shares command validation; 4 players max; 60Hz simulation, 20Hz snapshots; world-scoped profiles, LAN first, dedicated headless and online signaling/ICE/relay later. 20-byte app header, explicit little-endian fields, no raw structs; bounded packet reader/writer and schema validation. Separate protocol version, content hash, generator version, save schema. Validate ownership, range, state, epoch, command ID, rates; duplicate durable commands return stored result. Prediction local movement, reconciliation input replay, remote interpolation 100–200ms, extrapolation max 100ms; host validates all combat/economy. No kernel anti-cheat; malicious listen host remains trusted. Reliable bulk chunks separate priority/lanes where supported; avoid blocking critical messages. Auth and relay compatibility still require integration tests.

## 17. Build system and toolchain

CMake >=3.28 project minimum (proposed), C++23 target requirement, CXX_STANDARD_REQUIRED ON, CXX_EXTENSIONS OFF. GCC 14+ / Clang 18+ on Linux and MSVC VS 2022 recent toolset (19.40+ candidate) on Windows; actual feature probes and CI determine floor. Avoid C++ modules and compiler-specific experimental features initially. Ninja single-config presets linux-debug, linux-release, linux-asan, linux-tsan, windows-debug, windows-release; local user overrides ignored. vcpkg toolchain via environment path, pinned baseline. Build targets added incrementally, no phantom source files. Example commands after presets exist: `cmake --preset linux-debug`, `cmake --build --preset linux-debug`, `ctest --preset linux-debug --output-on-failure`. Options THJ_BUILD_CLIENT, THJ_BUILD_SERVER, THJ_BUILD_TESTS, THJ_ENABLE_NETWORK, THJ_ENABLE_PROFILING. Offline foundation must build with network disabled. Windows DLL packaging and Linux baseline compatibility smoke tests; do not assume Arch-built binaries run on all Linux distributions.

## 18. Testing and quality gates

Catch2 + CTest. Unit: math, ECS, collision, codec, stats, loot, conditions. Integration: SQLite transactions, migrations, backups, asset registry, loopback transport. Simulation: deterministic seeded generation checksums, replay command sequences, world changes. Property tests: stale handles, conservation, idempotency, no negative balances, malformed inputs. Fuzz packet and save/content readers with libFuzzer where supported. ASan/UBSan on Linux, TSan separate job (not combined with ASan), Windows smoke test and available sanitizers. Network impairment via simulated transport and tc netem; four-player two-hour soak. Golden snapshots use canonical serialized state, not raw struct bytes. Test crash between commit and ack, concurrent trade, reconnect, full inventory, chunk boundary, 0/negative inputs. CI blocks regression; benchmark gates only after measured baseline.

## 19. CI/CD and release

GitHub Actions PR/push: format check -> Linux GCC/Clang + Windows MSVC configure/build/test -> headless test -> asset manifest validation -> sanitizer jobs; cache keyed by compiler, triplet, manifest and baseline. Release tags build clean artifacts, bundle runtime data, required DLL/shared dependencies and licenses, produce SHA-256 checksums, run smoke tests on clean environments. No credentials committed; least permissions and pinned action revisions. Build on supported Linux baseline, not only Arch. Separate client/server artifacts; no automatic production deploy until explicit infrastructure exists.

## 20. Coding conventions

Namespace `thj`; types PascalCase, functions and local variables camelCase, members trailing underscore, constants kPascalCase, filenames PascalCase.hpp/.cpp for C++ and snake_case for data. 4 spaces, no tabs, clang-format (100 column candidate), include guards or pragma once consistently, self-contained headers, explicit includes, no using namespace in headers. Prefer value semantics, const correctness, std::optional/std::expected for recoverable failures when supported, exceptions caught at app boundaries; assertions for invariants, not validation of external input. Warnings GCC/Clang -Wall -Wextra -Wpedantic and MSVC /W4; warnings-as-errors for first-party CI only. Document units (px, ticks, grams, Mon), ownership and threading contracts. Conventional Commits, ADRs for impactful decisions; no secrets, saves, build products or generated caches committed. Source licensing decision still required; asset rights separate.

## 21. Performance budgets and diagnostics

60 FPS / 16.67ms frame target; server tick p95 <=12ms; vertical slice RAM <512 MiB target; textures 96 MiB, decoded audio 32 MiB, 512 particles, 16 lights, 32 voices; upload <=10 KiB/s download <=50 KiB/s normal gameplay per client; profile on i5-8250U/UHD620/12GB. Track tick backlog, draw calls, chunk build latency, DB commit latency, queue sizes, memory, snapshot age, RTT and loss. Structured logs with timestamps, subsystem, severity, world/session correlation; redact tokens. Optional Tracy after measurement. Do not assume budgets proven. Degrade cosmetic quality first, never silently skip authoritative actions.

## 22. Implementation sequence

1. Build foundation: minimal client opens SDL window, headless core tests, reproducible presets and CI.
2. Core: clock, ECS, commands/events, math, input and renderer.
3. World: tile collision, chunks, deterministic generation, camera and asset pipeline.
4. Gameplay: Aokawa combat, progression, inventory, quests; save/migrations.
5. Networking: local transport, LAN 2 then 4 players, prediction/replication, reconnect.
6. Online infrastructure, release packaging and optimization after vertical slice. Detailed scope locked in Step 0.13.

## 23. Acceptance criteria

- Linux and Windows build and tests succeed from clean checkout with pinned dependencies.
- Headless server and simulation tests do not initialize graphics/audio.
- Fixed timestep independent of render FPS; pause does not accumulate debt.
- ECS rejects stale handles; structural changes and event dispatch ordered.
- Physics tests cover diagonal movement, swept collision, realm isolation and chunk boundaries.
- Asset decode/upload lifecycle obeys thread ownership; failed assets handled safely.
- Input rebinding/hotplug/focus loss and audio buses work.
- Job cancellation, bounded queues and revision checks prevent stale writes.
- SQLite commit/ack ordering, rollback, idempotency, migration and backup recovery tested.
- Protocol bounds, replay rejection and cross-platform network tests pass.
- CI, sanitizers, fuzz tests, license manifest and release smoke tests pass.
- Profile actual hardware before expanding scope.

## 24. Official technical references

- https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
- https://wiki.libsdl.org/SDL3/Libraries
- https://wiki.libsdl.org/SDL3/SDL_RenderGeometry
- https://wiki.libsdl.org/SDL3/SDL_SetTextureScaleMode
- https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/BUILDING.md
- https://sqlite.org/wal.html
- https://sqlite.org/pragma.html
- https://www.sqlite.org/backup.html

## 25. Git checkpoint

```bash
git status
git add docs/
git commit -m "docs(gdd): finalize art direction and define technical architecture"
git push origin main
```

Design only. No C++ implementation, dependency installation, compilation, benchmark or repository mutation performed. Exact dependency pins, toolchain compatibility, online relay/authentication implementation and performance validation remain open. Step 0.13 will lock MVP scope.
