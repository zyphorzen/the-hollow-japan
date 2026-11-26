## 02 / Lore & Storyline

### Tiga dimensi

| Realm        | Peran                                          |
| ------------ | ---------------------------------------------- |
| Utsushiyo    | Mortal Realm: dunia manusia                    |
| Kakureyo     | Spirit Realm: yōkai, leluhur dan roh kuno      |
| Takamagahara | Celestial Realm: dewa dan penjaga keseimbangan |

### Sejarah dunia

| Era              | Peristiwa                                                    |
| ---------------- | ------------------------------------------------------------ |
| Age of Harmony   | Manusia, roh dan dewa hidup seimbang.                        |
| Age of Clans     | Kerajaan berkembang; kekuatan spiritual dimanfaatkan.        |
| Forbidden Ritual | Ritual terlarang merusak segel antardimensi.                 |
| Great Hollowing  | Dimensi bertumpang tindih; desa hilang dan korupsi menyebar. |
| Age of Hollow    | Dua puluh tahun kemudian, petualangan dimulai.               |

### The Hollow Wanderer

Karakter buatan pemain dengan nama, penampilan, gender dan origin yang dapat dipilih. Hollow Mark memungkinkan Realm Traversal, dengan risiko Hollow Corruption. Background memberi bonus awal dan dialog, bukan batas class.

| Background        | Keunggulan awal |
| ----------------- | --------------- |
| Fallen Samurai    | Swordsmanship   |
| Wandering Shinobi | Stealth         |
| Shrine Disciple   | Spiritual Arts  |
| Village Hunter    | Archery         |
| Merchant’s Child  | Trading         |
| Orphan Wanderer   | Survival        |

## 02 / Faction dan konflik

| Faction           | Ideologi dan tujuan                                                      |
| ----------------- | ------------------------------------------------------------------------ |
| Crimson Shogunate | Ketertiban militer; menutup gerbang dan menghancurkan ancaman spiritual. |
| Moonveil Order    | Mengembalikan keseimbangan manusia dan roh.                              |
| Shadow Lotus      | Memanfaatkan kekuatan Hollow untuk tatanan baru.                         |
| Wandering Blades  | Aliansi independen pengembara, pedagang dan pemburu.                     |
| Hollow Covenant   | Penyatuan dimensi sebagai evolusi berikutnya.                            |

### The Hollow Sovereign

Mantan penjaga gerbang spiritual yang kehilangan identitas dan ingatan setelah bencana. Ia ingin menghapus batas dimensi untuk mengakhiri penderitaan. Pemain dapat melawan, menyegel, menyelamatkan atau mendukungnya.

### Main storyline

| Chapter                     | Tujuan                         |
| --------------------------- | ------------------------------ |
| Prologue — The Awakening    | Menemukan Hollow Mark          |
| I — Whispers of the Dead    | Menyelidiki gangguan spiritual |
| II — Land of Broken Shrines | Menemukan segel kuno           |
| III — War of the Clans      | Menghadapi konflik faction     |
| IV — Beyond the Veil        | Menjelajahi dunia roh          |
| V — The Forgotten Truth     | Mengungkap penyebab bencana    |
| VI — Eclipse of the Realms  | Menghadapi penyatuan dunia     |
| VII — The Hollow Sovereign  | Menentukan nasib dunia         |
| Epilogue — A World Reborn   | Mengalami konsekuensi pilihan  |

## 02 / Narrative mechanics

### Opening sequence

Karakter terbangun di Forgotten Shrine saat hujan, dengan ingatan beberapa hari terakhir menghilang dan simbol spiritual pada tangan. Suara dari torii berkata: “You stand between two worlds, wanderer. Neither will accept you. Yet both will need you.” Pemain menuju Aokawa Village untuk tutorial natural.

### Ending dan konsekuensi

| Ending        | Hasil                                  |
| ------------- | -------------------------------------- |
| Separation    | Dimensi dipisahkan kembali.            |
| Harmony       | Manusia dan roh hidup berdampingan.    |
| Ascension     | Dimensi menyatu menjadi realitas baru. |
| Oblivion      | Korupsi mengambil alih dunia.          |
| Hidden Ending | Syarat rahasia; belum dirinci.         |

Post-game exploration tetap tersedia. Ending dipengaruhi keputusan, reputasi dan korupsi, bukan semata faction.

### Story state

faction_reputation, npc_relationships, hollow_corruption, world_flags, story_flags, quest_states, realm_stability.

### Hollow Corruption

Rentang 0–100: Pure (0–19), Spirit Touched (20–39), Unstable (40–59), Corrupted (60–79), Near Hollow (80–99), Fully Hollow (100). Prototype bonus spiritual damage = floor(corruption × 0,4)%; penalti physical defense = floor(corruption × 0,25)%. NPC menjadi curiga mulai 40, takut atau hostile mulai 80. Nilai belum diuji.

### Co-op narrative

Host menentukan keputusan cerita dan menyimpan world progression; karakter dan personal quest per pemain; individual loot, enemy scaling, friendly fire nonaktif, PvP di luar MVP. Tactical pause offline menghentikan simulasi, online membutuhkan persetujuan seluruh pemain.
