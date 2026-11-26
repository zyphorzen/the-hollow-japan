## 03 / World geography

Ashihara: kepulauan fiktif, finite seamless open world, delapan region, 3 realm, empat musim, fast travel melalui shrine, regional difficulty dengan conditional scaling.

### Peta konseptual (arah utara di atas)

| Posisi                      | Region                    |
| --------------------------- | ------------------------- |
| Utara                       | 1. Yukimine               |
| Utara-tengah                | 2. Shinrin                |
| Barat-tengah                | 3. Kusanohara             |
| Timur-tengah                | 4. Akagane                |
| Tengah-selatan              | 5. Aokawa (starting area) |
| Barat daya                  | 6. Shiohama               |
| Tenggara                    | 7. Yomigahara             |
| Pulau lepas pantai tenggara | 8. Hollow Isles           |

### Region 1–4

| Region / level     | Konten                                                                                                                              |
| ------------------ | ----------------------------------------------------------------------------------------------------------------------------------- |
| Yukimine / 25–40   | Yukimori Village; Moonveil; Ice Yōkai, Snow Oni; Temple of Eternal Winter; Frozen Guardian; Frost Crystal, Silver Ore; badai salju. |
| Shinrin / 8–18     | Komorebi Village; Moonveil; Tengu, corrupted Kitsune; Forest of Lost Souls; Nine-Tailed Guardian; Spirit Wood, Moonleaf; ilusi.     |
| Kusanohara / 10–20 | Kusanagi Town; Wandering Blades; bandit, beast, spirit; Buried Fortress; Ancient War Beast; herbs, wheat, iron; perdagangan.        |
| Akagane / 20–35    | Akagane Fortress; Shogunate; Rogue Samurai, Flame Oni; Crimson Forge; Ashen General; Obsidian, Crimson Ore; panas ekstrem.          |

## 03 / Region, biome dan realm

| Region / level     | Konten                                                                                                                      |
| ------------------ | --------------------------------------------------------------------------------------------------------------------------- |
| Aokawa / 1–10      | Aokawa Village; netral; Wild Boar, Minor Spirit; Forgotten Shrine; Hollow Stag; bamboo, wood, herbs; tutorial.              |
| Shiohama / 12–25   | Shiohama Port; Wandering Blades; Water Spirit, bandit; Sunken Sanctuary; Tidal Serpent; fish, pearl, seaweed; pasang surut. |
| Yomigahara / 35–50 | Hidden Lotus Enclave; Shadow Lotus; Hollow Samurai, Wraith; City of the Dead; Nameless Ronin; Hollow Shard, Cursed Essence. |
| Hollow Isles / 50+ | Hollow Citadel; Covenant; Hollow Sentinel, Void Oni; Palace of the Hollow Sovereign; Void Crystal, Ancient Relic; endgame.  |

### Biome

Meadow, Sakura Forest, Bamboo Forest, Ancient Forest, Grassland, Farmland, Mountain, Snowland, Volcanic, Coastal, Swamp, Corrupted Land, Void. Biome menentukan resource, musuh, vegetasi, audio, cuaca dan bahaya seperti racun, dingin, panas, ilusi atau korupsi.

### Realm traversal

Utsushiyo dan Kakureyo berbagi koordinat geografis tetapi memiliki terrain, collision, entity, resource, quest dan state terpisah. Takamagahara berupa area instanced, bukan open world penuh. Perpindahan dilakukan pada lokasi tertentu.

### Akses

Level region bersifat rekomendasi, bukan hard gate. Kemampuan, item dan story flags dapat menjadi syarat lokasi khusus.

## 03 / Aokawa dan dungeon

### Starting village

| Fasilitas           | Fungsi                      |
| ------------------- | --------------------------- |
| Forgotten Shrine    | Spawn, realm traversal      |
| Village Square      | NPC dan festival            |
| Blacksmith          | Craft dan upgrade senjata   |
| General Store       | Jual beli                   |
| Notice Board        | Quest dan bounty            |
| Player House        | Storage dan resting         |
| Village Inn         | Resting dan informasi       |
| Training Grounds    | Tutorial combat             |
| Farm / Fishing Spot | Gathering, farming, fishing |

NPC: Elder Haruto (kepala desa), Akari (shrine maiden), Takeshi (blacksmith), Hana (herbalist), Ren (swordsman), Yuki (merchant), Sora (fisherman), Kuro (spirit companion). NPC memiliki jadwal, rumah, dialog, hubungan dan reaksi terhadap korupsi.

### Dungeon types

| Jenis      | Desain                                   |
| ---------- | ---------------------------------------- |
| Story      | Handcrafted, puzzle dan boss             |
| Procedural | Seeded rooms, connections, enemies, loot |
| Spirit     | Akses dari Spirit Realm                  |
| World      | Lokasi tersembunyi saat eksplorasi       |
| Endgame    | Modifier dan reward khusus               |

Generator dungeon: room graph, BSP, template placement, collision dan connectivity validation. Entrance harus terhubung ke boss; reward dan puzzle ditempatkan pada ruangan sesuai aturan.

## 03 / World engine dan chunk

| Parameter                 | Target awal                                             |
| ------------------------- | ------------------------------------------------------- |
| Ukuran prototype          | 256 × 256 tiles                                         |
| Ukuran dunia akhir        | 4096 × 4096 tiles per Mortal Realm                      |
| Tile / chunk              | 16 px / 32 × 32 tiles (512 × 512 px)                    |
| Jumlah chunk dunia akhir  | 16.384                                                  |
| Raw tile memory           | 32 MiB per layer; 160 MiB untuk 5 layer mentah          |
| Active / preload / unload | Radius 1 / 2 / 3 chunks (Chebyshev)                     |
| Active / loaded           | 9 / 25 chunk inti per pemain, ditambah cache hysteresis |

### Lifecycle

Unloaded → Queued → Generating/Loading → Ready → Active → Inactive → Saving (jika dirty) → Unloaded. Inactive dapat kembali Active. Loading asinkron; SDL rendering terpisah dari simulasi.

### Tilemap

Layer: Ground, Decoration, Objects, Overhead, Effects. Collision, navigation, entity dan trigger terpisah. TileRegistry menyimpan metadata statis: tile_id uint16_t, biome_id uint8_t, walkable, swimmable, movement_cost, damage_per_second, light_blocking, interaction_type, footstep_sound.

### Koordinat

World pixel → tile = floor(world / 16); chunk = floor(tile / 32); local tile = floor-mod(tile, 32). Contoh (1250, 790) → tile (78, 49) → chunk (2, 1) → local (14, 17). Validasi batas dunia dan realm ID diperlukan.

## 03 / Generation dan persistence

### Generation pipeline

World Seed → Macro Geography → Climate Maps → Biome Classification → Handcrafted Placement → Terrain Generation → Feature Generation → Connectivity Validation → Persistent World.

Lokasi cerita, kota, desa, kuil, boss arena dan jalan utama bersifat handcrafted. Vegetasi, resource, minor cave/dungeon, encounter, terrain detail dan non-story ruins dihasilkan secara procedural. Macro geography menggunakan region mask.

| Algoritma                         | Kegunaan                     |
| --------------------------------- | ---------------------------- |
| OpenSimplex / gradient noise, fBm | Elevation dan detail terrain |
| Voronoi                           | Variasi region dan biome     |
| Cellular automata                 | Cave                         |
| BSP / room graph                  | Dungeon                      |
| A\*                               | Jalan dan navigasi           |
| Weighted sampling                 | Spawn dan resource           |
| Autotiling                        | Transisi terrain             |

### Determinisme

Stable hash dari world seed, region ID, realm ID, chunk X/Y, generation layer dan generator version menghasilkan chunk seed. Hindari RNG global dan waktu sistem; sampling terrain lintas batas menggunakan koordinat global. Uji checksum lintas Linux/Windows.

### Persistence

Base World + World Delta. Simpan perubahan resource, chest, boss, bangunan, NPC, dungeon, korupsi dan quest. SQLite diusulkan; saves/world_001/{world.meta, world.db, backups/}. Tabel: world_metadata, chunk_deltas, world_entities, players, inventories, quest_states, world_flags, npc_states, faction_states. Generator version dan migrasi save wajib dipertimbangkan.

## 03 / Simulasi dan networking

### World clock

48 menit real time per hari game; 2 menit per jam; 28 hari per musim; 112 hari per tahun. Spring: sakura/festival; Summer: badai/fishing; Autumn: panen; Winter: salju/dingin. NPC dan spirit mengikuti waktu.

### Weather & world events

Clear, Cloudy, Rain, Thunderstorm, Fog, Snow, Blizzard, Spirit Storm, Hollow Eclipse. Event: Bandit Raid (security rendah), Yōkai Invasion (stability rendah), Traveling Merchant, Sakura Festival, Spirit Migration, Faction War, Hollow Eclipse, Village Reconstruction. Event besar memiliki cooldown.

### Multiplayer world synchronization

Host-authoritative: host mengontrol seed, generator version, clock, weather, spawn, NPC, combat damage, gathering, perubahan dunia dan persistence. Client mengirim input serta melakukan rendering dan efek visual lokal. Base terrain dapat digenerate lokal setelah validasi versi.

Interest management, chunk subscriptions, entity replication, snapshots, delta updates, reference counting, prediction dan reconciliation. Host mempertahankan gabungan chunk yang dibutuhkan semua pemain. Pemain dapat terpisah region atau realm, bergantung batas performa.

### Performance target

| Metric                | Target (belum diuji)                                                                                    |
| --------------------- | ------------------------------------------------------------------------------------------------------- |
| FPS / frame budget    | 60 / 16,67 ms                                                                                           |
| Internal resolution   | 480 × 270; integer scaling; nearest-neighbor                                                            |
| RAM vertical slice    | < 512 MiB                                                                                               |
| Simulation / snapshot | 60 Hz / 20 Hz                                                                                           |
| Optimisasi            | Culling, texture atlas, batching, spatial partition, pooling, dirty flags, async loading, AI throttling |

## 03 / Arsitektur dan roadmap

### World module

src/world/ terdiri atas core (World, WorldConfig, WorldCoordinates), chunk (Chunk, ChunkManager, ChunkLoader, ChunkState), tile (Tile, TileMap, TileLayer, TileRegistry), generation (WorldGenerator, TerrainGenerator, BiomeGenerator, DungeonGenerator, FeatureGenerator, NoiseGenerator), region (Region, RegionManager, Biome), realm (Realm, RealmManager, RealmTransition), simulation (WorldClock, WeatherSystem, SeasonSystem, SpawnSystem, WorldEventSystem), navigation (NavigationGrid, Pathfinder), persistence (WorldRepository, ChunkSerializer, WorldDelta).

World simulation tidak bergantung langsung pada SDL renderer. Data-driven assets/data: world.json, regions.json, biomes.json, tiles.json, weather.json, seasons.json, spawn_tables.json, world_events.json. Assets/maps: regions, villages, dungeons, realm_templates.

### Stack usulan

C++23, SDL3, CMake, Ninja, GCC/Clang/MSVC, Tiled, LibreSprite/Aseprite, JSON/nlohmann-json, SQLite, FastNoiseLite/custom, Catch2, spdlog, Git, GitHub Actions. Networking library belum dipilih.

### Milestones

| Milestone             | Deliverable                                                                                                    |
| --------------------- | -------------------------------------------------------------------------------------------------------------- |
| Technical prototype   | 256² tiles, streaming, rendering, movement, camera, collision, deterministic terrain.                          |
| Aokawa vertical slice | Desa, 1 forest, 2 dungeon, 5–8 NPC, 3–5 enemy, 1 boss, inventory, crafting, quest, save/load, realm traversal. |
| Multiplayer alpha     | Host/client, co-op 2–4, entity/chunk/combat sync dan shared persistence.                                       |
| Full expansion        | 8 region, Spirit Realm, advanced dungeon, faction, seasons, branching story, endgame.                          |
