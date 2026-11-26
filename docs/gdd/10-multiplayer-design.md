# Step 0.10 — Multiplayer Design

Version: 0.10.0 | Status: Draft — awaiting approval

## 1. Baseline and decisions

Offline singleplayer and 2–4 player co-op on Linux/Windows share a C++23 simulation independent of SDL3. Listen server is the release baseline; headless dedicated server is a subsequent milestone. Host-authoritative world, combat, economy and quests; friendly fire disabled, no PvP or host migration in MVP. Simulation 60 Hz, snapshots 20 Hz, input up to 60 Hz, SQLite persistence. Use GameNetworkingSockets (GNS) behind an INetworkTransport interface; pin tested version at Step 0.12. GNS supports reliable/unreliable UDP messages, encryption and optional ICE. SDL_net offers basic TCP/UDP; ENet offers reliable UDP but leaves encryption/authentication/lobbies to the application. GNS does not automatically grant Steam matchmaking or relay service. References: https://github.com/ValveSoftware/GameNetworkingSockets ; https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/README_P2P.md ; https://github.com/lsalzman/enet/blob/master/docs/mainpage.dox ; https://wiki.libsdl.org/SDL3_net/FrontPage .

## 2. Unified architecture

Targets: thj_core (ECS/math), thj_simulation (authority), thj_protocol (codec), thj_network (transport), thj_client (SDL presentation), thj_server (headless), thj_tests. Offline uses in-memory command transport; listen server combines local client and authority; dedicated runs authority without rendering/audio. Local host follows same command validation as remote clients. Only simulation thread mutates world; bounded queues connect transport, DB worker and simulation. Fixed tick accumulator; never silently skip gameplay ticks; overload caps catch-up, sheds optional work, logs overruns and rejects new sessions if needed. Paused gameplay does not pause transport, wall clock or authentication timers.

## 3. Connectivity, lobby and handshake

LAN direct IP and UDP port 27026 (configurable), then LAN discovery via broadcast or discovery service; validate discovery response and never trust it as authentication. Direct online IP/domain precedes invite-code signaling. Private lobby MVP; friends-only/public require identity and discovery infrastructure. Lobby state Created -> Waiting -> Ready -> Loading -> InGame -> Closing -> Closed. 2–4 total slots, host counts for listen mode; dedicated allows four clients. Host controls readiness, access policy and join-in-progress; joining during boss encounter deferred to safe checkpoint.

Connection sequence: encrypted transport -> ClientHello (protocol/content/generator versions) -> ServerHello and authentication challenge -> authenticated proof -> JoinAccepted -> WorldManifest -> full baseline and chunk readiness -> WorldReady -> SpawnConfirmed. Use verified server identity and standard auth; private LAN can use out-of-band verified host fingerprint and high-entropy invite credential. Online uses short-lived signed lobby tickets via HTTPS and authenticated connection identity. Password alone is not identity. Reject mismatched protocol/content/generator, expired tickets, over-capacity sessions, invalid character ownership. Session ID and epoch scoped to connection; reconnect token random, expiring, rotated and bound to player/world. Never expose tokens in logs.

## 4. Transport and protocol

Reliable messages: action intents, inventory, quest decisions, entity lifecycle, chunk baselines, chat, lobby. Unreliable sequence-validated messages: input samples and snapshots. Separate reliable control and bulk lanes where supported, so chunk loads cannot block critical commands; bounded queues and backpressure. HTTPS/TCP only for external services. GNS handles transport reliability, fragmentation and encryption; do not invent crypto or reimplement UDP reliability.

Application header (20 bytes): version u16, type u16, payload size u32, sequence u32, simulation tick u32, session epoch u32. Fixed little-endian field encoding, no raw structs or pointers. Session identity comes from authenticated connection. Message ID ranges: 0x01xx handshake, 0x02xx lobby, 0x03xx input, 0x04xx entities, 0x05xx combat, 0x06xx chunks, 0x07xx economy, 0x08xx quests, 0x09xx sessions, 0x0Axx chat. Input logical payload: inputSequence u32, clientTick u32, moveX/moveY i8, actionFlags u16, aimAngle u16 (14 bytes serialized). Discrete actions use separate unique command IDs; flags only represent held state. Explicit field bounds, reject unknown required message, invalid enums, truncated/trailing payloads, excessive collections. Sequence wrap uses modular comparisons within half-range; epoch prevents stale-session replay. Ordinary realtime application messages target <= 1000 bytes including header; bulk chunks sliced into <= 16 KiB reliable messages, max reassembled chunk transfer 256 KiB, with bounded outstanding count and timeout. These are application budgets, not MTU guarantees. JSON only authored data; binary on wire. Position int32 fixed point 1/16 px; aim u16 maps full turn; client time never trusted for authority.

## 5. Prediction and reconciliation

Client samples movement at fixed 60 Hz, sends up to 60 Hz with last 2 redundant samples. Server validates normalized input, sequence, tick window, ownership, movement state, stamina and collision. Process at most one movement input per player per server tick, never accelerate to catch up; missing samples hold last intent up to 6 ticks then neutral. Snapshot includes authoritative tick, last processed input sequence, position, velocity and relevant movement state. Client predicts own movement using same movement/collision rules; stores 256 tick ring buffer. On snapshot, reset predicted state to authoritative state, discard acknowledged inputs, replay remaining inputs. Smooth visual correction over ~100 ms if error <= 8 px, snap if larger or realm/teleport/collision invalid. Never smooth authoritative collision state. If history unavailable request resync. Predict presentation of attacks, not damage, loot, parry result or economy. Discrete action intent reliable, expires when too old; server validates timing, never blindly rewinds world. MVP has no rollback combat; optional bounded lag compensation for ranged hit tests later after fairness tests.

## 6. Snapshot interpolation and replication

20 Hz = 50 ms spacing. Remote rendering target server-time minus 100 ms initially, adaptive 100–200 ms. Interpolate between known snapshots (position linear, facing shortest angle); extrapolate at most 100 ms then hold. Teleports/realm transitions snap. Estimate clock offset using ping/pong and monotonic clocks, track RTT/jitter/loss. Entity network ID plus generation, separate persistent UUID. Reliable spawn/despawn and realm transition; unreliable snapshots include entity generation and baseline revision; discard stale or unknown entity updates or briefly buffer bounded pending updates. Full baseline on join/recovery; deltas only against acknowledged baseline, periodic keyframe ~2s; missing baseline requests reliable resync. Prioritize nearby combat/player entities, use lower update frequencies for idle NPCs. Never replicate hidden secrets or private inventories to other players. No raw ECS pointers on wire.

## 7. Chunk synchronization

Chunk key = world ID, realm ID, x, y, generator version. 32x32 tiles, active radius 1, preload 2, unload outside 3; host keeps union for up to 4 players and reference counts. Manifest includes world seed, content hash, region masks version, generator version. Client generates deterministic base terrain, compares canonical chunk checksum; host sends authoritative delta and revision or full fallback baseline if mismatch. Checksums detect mismatch, not authenticate content. Revisioned deltas apply only against expected base; gaps trigger resync. Subscribe/ready handshake ensures collision data before player enters chunk. Realm transitions load destination before authoritative teleport; unload invalidates subscriptions and stale packets. Never trust client generated collision or resource state. Cap outstanding chunk loads and prioritize gameplay traffic. World persistence stores Base + Delta, same as Step 0.3.

## 8. Latency and performance budgets

Target RTT <= 100 ms ideal, 100–200 playable with prediction, >200 degraded; not guaranteed. Jitter 0–50 ms, loss 0–5% tested; 10% stress. 60 Hz authority budget 16.67 ms, target p95 <= 12 ms on reference machine in vertical slice. 20 Hz snapshots, aim <= 10 KiB/s client upload and <= 50 KiB/s download sustained normal gameplay excluding loading/relay overhead; profile actual transport bytes. 4-player dedicated server egress scales with clients; listen server has at most 3 remote clients. Limit per-client reliable backlog 1 MiB, throttle bulk and disconnect persistently stalled peers. Measure tick time, queue depth, RTT, loss, reconciliation error, bandwidth, chunk loading and DB commit latency. All targets untested.

## 9. NAT traversal and online infrastructure

LAN first; direct public address/port forwarding when available; public IPv6 subject to firewall; optional ICE/STUN hole punching for online private sessions. Signaling exchanges authenticated candidates, STUN discovers public mappings, relay forwards traffic when direct connection fails. CGNAT/symmetric NAT/firewall can prevent direct connection. Open-source GNS ICE requires appropriate build dependencies and signaling; verify TURN or other relay interoperability in spike, do not assume Steam Datagram Relay access. Provide self-hosted dedicated server as fallback. Online service: HTTPS lobby/ticket endpoint, authenticated signaling, STUN and compatible relay as needed; short-lived credentials, quotas, TLS, abuse prevention. No fixed cloud vendor/cost yet. LAN/offline must work without service. Direct connections reveal peer IP; relay can reduce exposure if correctly configured.

## 10. Disconnect, reconnect and save policy

Heartbeat/transport health uses monotonic wall time. Warn after ~3s silence; connection timeout target 10s (transport settings tested). On loss, neutral input within 6 ticks of missing data; character remains subject to combat for initial 10s and cannot gain logout invulnerability. Reserve slot for 60s after disconnect; alive character outside combat can be safely suspended after timeout; combat disconnect resolves via existing combat/death rules, with bounded suspension policy to prevent indefinitely active abandoned entity. Reconnect proves identity, rotates token/epoch, restores authoritative character and sends full snapshot, inventory, quest, chunk state. Do not replay committed economic commands; return stored result for same command ID; conflicting payload rejected. Host shutdown: reject new commands, finish/rollback transactions, persist checkpoint, notify clients, close. Host crash: recover committed SQLite state, discard uncommitted state; no automatic host migration MVP. Guest may reconnect when same host world returns.

Character progression stored by world + player ID, not trusting imported client save. Offline character may become host character when opening its own world; guests create or resume world-scoped profiles. No cross-world item/currency import/export in MVP; optional future controlled transfer. This refines earlier per-player progression: retained in host world across visits, not automatically copied to personal offline worlds. Shared story host-controlled, personal quests player-specific within world. Guest rewards remain in host-world profile. Economy transaction and reward ledgers share DB transaction; durable commit precedes acknowledgement. Periodic world checkpoint target 30s plus critical mutations immediately; backup before migration. Dedicated admin access separate from player privileges.

## 11. Security and anti-cheat

Threat model: malicious clients, malformed packets, replay, duplicate reward, speed/teleport, fake damage, inventory tampering, unauthorized joins, spam, denial of service, compromised host. Validate all client intentions, ownership, realm, range, cooldown, resource and state. Rate limit by message type and authenticated connection, bound parsing and allocations, reject invalid payloads and excessive queues. Action command IDs and world-scoped transaction ledger guarantee idempotency; compare request hash on duplicates, atomically commit result with state. Host computes RNG and loot. Use encryption and verified peer identity, expiring tickets, no plaintext secrets or homegrown crypto. Restrict admin interface to authenticated authorized operators; no arbitrary script execution. Record structured audit events with no tokens or sensitive payloads. Listen host can modify its own world/executable; no claim of cheat-proof peer hosting. Dedicated server also only trusted if operated by trusted operator. No invasive kernel anti-cheat. Chat length/rate limits, mute/kick controls and optional ban list. Security testing includes fuzzing, unauthorized commands, replay and resource exhaustion.

## 12. Tactical pause and co-op contracts

All active players must agree; vote expires after 10 wall-clock seconds; membership change cancels pending vote. Approved pause freezes simulation, cooldowns, projectiles and world clock while network/UI stay active. Host may resume, any player may request resume; cap online pause at 60 wall-clock seconds initially and resume on disconnect to prevent deadlock. Queue <=3 actions/player; validate again on resume. Shared boss scaling locked at encounter start (1,1.6,2.1,2.5); revive 3s, downed 20s; rewards per eligibility, shared objective event once. Full resync after pause if necessary.

## 13. Module layout and config

```text
src/network/
  transport/INetworkTransport.hpp
  transport/LocalTransport.hpp
  transport/GnsTransport.hpp
  protocol/MessageHeader.hpp
  protocol/PacketReader.hpp
  protocol/PacketWriter.hpp
  session/SessionManager.hpp
  session/LobbyService.hpp
  replication/EntityReplication.hpp
  replication/SnapshotBuffer.hpp
  replication/InterestManager.hpp
  replication/ChunkReplication.hpp
  prediction/ClientPrediction.hpp
  prediction/Reconciliation.hpp
  security/CommandValidator.hpp
  security/RateLimiter.hpp
src/server/ServerApplication.hpp
```

Config example (proposed):

```json
{
  "port": 27026,
  "max_players": 4,
  "tick_hz": 60,
  "snapshot_hz": 20,
  "input_hz": 60,
  "interpolation_ms": 100,
  "max_extrapolation_ms": 100,
  "reconnect_grace_seconds": 60,
  "pause_vote_timeout_seconds": 10,
  "protocol_version": 1,
  "allow_join_in_progress": true
}
```

## 14. Testing and release gates

Unit tests: codec roundtrip/golden bytes across Linux/Windows, malformed/truncated payloads, integer overflow, sequence wrap, input normalization, authority validation, snapshot interpolation, reconciliation, chunk checksum and revision. Integration: headless 1/2/4 clients, local vs network command parity, lobby rejection, late join, realm transition, NPC and boss replication, transaction replay and reconnect, host crash before/after commit. Network matrix RTT 0/50/100/200/300ms; jitter 0/20/50ms; packet loss 0/1/5/10%; reordering, duplicates, bandwidth throttling; use deterministic simulated transport for CI, Linux netem for integration, Windows/Linux actual LAN tests. NAT tests: direct LAN, IPv4 public, IPv6, CGNAT/relay fallback; verify encryption and identity failure cases. Soak 4 clients 2h initially, monitor memory, queues and tick percentiles; no unbounded growth. Fuzz decoder with sanitizers. Test pause voting/disconnect, trade revision, simultaneous loot and reward uniqueness. No tests yet executed.

Milestones: (1) shared headless simulation + local transport; (2) GNS LAN connect, input/snapshot, two clients; (3) four players + prediction + chunk sync + authoritative combat/economy; (4) lobby/reconnect/online direct; (5) ICE/relay and dedicated deployment; (6) soak/security/cross-platform release gates. Online infrastructure and dedicated release timing depend on Step 0.13 scope.

## 15. Acceptance criteria

- [ ] Offline play does not require network service.
- [ ] Linux/Windows can join compatible LAN session, max four players.
- [ ] Invalid protocol/content/auth rejected.
- [ ] Movement cannot exceed authoritative speed or collision limits.
- [ ] Prediction reconciles without modifying authoritative state.
- [ ] Snapshot loss, stale entity and chunk revision handled safely.
- [ ] Host validates combat, inventory, quests, and reward claims.
- [ ] Reconnect/crash cannot duplicate committed transaction.
- [ ] Guest profiles isolated from local world progression.
- [ ] Tactical pause vote and timeout do not deadlock.
- [ ] Direct/relay fallback verified before advertising online support.
- [ ] Four-player soak and latency budgets measured.
- [ ] Decoder fuzzing and cross-platform CI pass.

All architecture, port, rates, limits, timeout and budget values are proposed. GNS capabilities verified from official documentation, but dependency build, signaling/relay integration, security, benchmarks and gameplay have not been tested. Git checkpoint: `git commit -m "docs(gdd): finalize npc quests and define multiplayer architecture"`.
