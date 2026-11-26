# Step 0.11 — Art Direction & Audio

Version: 0.11.0 | Status: Draft — awaiting approval

## 1. Vision and baseline

Dark Japanese Fantasy — Atmospheric Pixel Art. Readability, atmosphere and consistency take precedence over decorative detail. 16-bit inspired, top-down three-quarter view, 16x16 tiles, 16x32 character canvas, eight movement directions and four animation directions. SDL3 client; 480x270 logical world canvas, 16:9, nearest filtering, integer scaling, 60 FPS target. LibreSprite/Aseprite and Tiled are authoring candidates; no external art or audio is bundled by this design. Utsushiyo is natural and warm, Kakureyo cold and supernatural, Takamagahara sacred and luminous. Fictional Japanese-inspired setting; avoid presenting invented rituals as authentic history.

## 2. Color system

Master palette (sRGB hex):

| Group   | Colors                                          |
| ------- | ----------------------------------------------- |
| Neutral | #161820 #252833 #393D48 #626875 #A8A8A3 #E8E0CC |
| Nature  | #20382D #35543C #50734A #76965C #A7B87C #D8D3A2 |
| Wood    | #392B29 #5C4036 #825B43 #AD8057 #D2AC7A #E7CBA3 |
| Sakura  | #542D40 #803C4C #B45665 #DA8290 #EDB0B7 #F7D7D8 |
| Spirit  | #1D203D #35325B #55467E #8264AD #B19ADB #D6C8F1 |
| Sacred  | #665237 #A1854A #C9A55A #E3C779 #F3E3AD #FFF6DB |

| Region       | Palette                                 |
| ------------ | --------------------------------------- |
| Aokawa       | #E4B0B8 #75915A #C6AD79 #855647 #566F50 |
| Shinrin      | #203B32 #37664C #68936C #A4BB77 #34514D |
| Yukimine     | #EDF3F4 #B6CAD8 #7F9AB5 #526B87 #344554 |
| Kusanohara   | #3E6746 #779C58 #B8C883 #D0B56D #796F49 |
| Akagane      | #322A2E #75403B #B6553D #DD8950 #F2BE68 |
| Shiohama     | #274E65 #467B92 #72ABBA #D8D5AF #A99B75 |
| Yomigahara   | #25282A #3E4241 #5B6250 #795C77 #AAA296 |
| Hollow Isles | #171727 #33264A #59417D #9165C1 #C0A6E5 |

Palette is guidance, not a hard color limit. Use top-left painted light, material-specific outlines, strong silhouettes, restrained clusters, ~12–20 colors per base character. Gameplay indicators always combine color with shape/text.

## 3. Environment and tiles

16x16 grid, ground/decoration/objects-overhead/effects logical layers (objects and overhead are distinct); foot-based Y-sort within relevant layers. Separate collision, navigation, triggers and resource metadata. Ground, transitions, roads, water, vegetation, architecture, interiors, dungeon, spirit and props atlas groups. Four-neighbor masks for simple transitions, 47-tile constrained eight-neighbor blob sets for complex terrain. Deterministic visual variation separate from gameplay RNG. Tiled source .tmx/.tsx; build exporter validates stable tile IDs, transforms, layer names, references, and cross-chunk border masks. Animated water and foliage are presentation-only. Roof/canopy fade must not change collision.

## 4. Sprite specification

Player/NPC 16x32; small animal 16x16; minor spirit 24x24; medium enemy 32x32; oni 48x48; Hollow Stag 96x96; large bosses 128x128+; icons 16x16; portraits 64x64. Visual canvas does not define collider (player baseline 12x10 foot collider). Use consistent original canvas, foot anchor, layer pivots and four cardinal directions; diagonals select dominant direction with hysteresis. Layer order is direction-dependent: shadow, base, clothing, hair/headgear, weapon, foreground effects (weapons may go behind body). Separate larger attack effect canvas. Do not mirror asymmetric equipment without explicit metadata.

## 5. Animation

Idle 4 frames/4 fps; walk 6/8; run 6/12; sprint 6/14; attack 6 nominal; dodge 5 nominal; hit 3; death 6; cast/interact variable; sleep 2–4; realm transition 8–12. Locomotion loops use presentation time; combat clips map onto authoritative startup/active/recovery ticks at 60Hz. Animation events may trigger cosmetics only; hit detection never depends on render frame. Pause freezes gameplay animation; UI animates separately. Sprite metadata records frame durations, pivots, direction, layer mapping, loop mode and optional cosmetic markers. Missing clip falls back to idle and logs in development; required production clips fail asset validation.

## 6. Texture atlas

PNG RGBA8, straight alpha, nearest filtering, no mipmaps initially, no rotated frames. Two-pixel edge extrusion plus at least two-pixel separation between extruded regions. 1024–2048 terrain/character/equipment/enemy pages; 1024 UI/VFX; individual boss pages as needed. 2048 square RGBA8 = 16 MiB uncompressed. Stable logical IDs independent of packing position; JSON metadata contains page, rect, original size, trim offset, pivot. Build deterministic ordering and content hashes. Load region/realm groups on demand; reference counted resources; GPU upload on render owner thread, CPU decoding on workers. Never destroy atlas while frame references remain. Rendering batches may not reorder alpha-blended Y-sorted objects incorrectly.

## 7. UI and typography

HUD: HP red with heart, stamina green with bolt, spirit blue with diamond, posture gold, corruption violet with separate numeric/shape indicators. Top-left resource bars; top-right minimap and world time; bottom-center 4 consumable slots and 4 skill slots; contextual interaction prompts; boss bar top-center. Inventory 32 slots (8x4), character/equipment panel, map, journal, dialogue, settings, lobby and tactical pause. Keep combat telegraphs visible; keyboard/gamepad focus navigation, remappable input, scalable text, colorblind-safe symbols, optional reduced motion, screen shake and flash toggles. World canvas integer scaled with letterbox; UI can render at native output resolution using logical layout and independent UI scale. At 1366x768, world uses 2x = 960x540 in strict integer mode; optional fit mode explicitly trades pixel precision. 1920x1080 = 4x, 1280x720 = 2x. Pixel-style headings; readable sans-serif body; candidate Noto Sans/Noto Sans JP for Latin/Japanese coverage; verify license for exact font files. UTF-8 localization keys, wrapping, fallback glyphs, no essential text baked into sprites; baseline 16px body at 1x UI reference with scale 100–200%. Text contrast target 4.5:1 for normal text. Use SDL_ttf candidate or equivalent font renderer, pin/test in 0.12.

## 8. Lighting and render pipeline

World ground -> decoration -> Y-sorted entities/objects -> overhead -> world lighting composite -> emissive/VFX -> upscale/present world -> UI. Ambient tint + additive radial lights with bounded intensity; basic darkness mask prototype via render targets and blending. Occlusion/shadow casting optional after profiling; advanced shaders may require SDL GPU/custom backend, not assumed available via basic SDL_Renderer. Use pixel-aligned camera sampling and nearest upscale; keep simulation positions fractional. Day/night 48 real minutes/day, time-driven tint; dusk warm, night indigo, dawn soft blue. Preserve enemy silhouettes and readable telegraph outlines at night; accessibility brightness option does not reveal hidden entities. Lighting cosmetic only unless explicit authoritative gameplay light metadata exists. Limit visible lights initially 16; cull offscreen.

## 9. VFX and weather

Particles: sword slash, impact, dust, sparks, spirit motes, healing, barrier, hollow aura, sakura petals, rain, snow, fog, lightning. Prototype cap 512 live particles total, pool/reuse, priority for combat indicators, offscreen culling. Weather driven by authoritative type/intensity/seed/time; clients generate local cosmetic particles, not per-particle packets. Clear/cloudy/rain/thunder/fog/snow/blizzard/spirit storm/eclipse. Gameplay effects (visibility restrictions, hazards, cold) are separate simulation rules. Lightning uses muted flashes; reduced-flash mode replaces abrupt flashes with smooth tint; no rapid strobing. Camera shake local, adjustable/off; audio cue alternatives for important events. Realm transition uses 0.3–0.6s fade around authoritative readiness; no fixed animation timer may bypass loading.

## 10. Music direction

Original instrumental Japanese-inspired fantasy score; koto/shamisen/shakuhachi/taiko-inspired timbres, restrained strings and pads. No copying melodies or copyrighted recordings. Themes: title (lonely motif), Aokawa day (warm), night (sparse), Shinrin (mystery), Yukimine (cold), Akagane (percussion), Shiohama (flow), Yomigahara (dissonance), Hollow Isles (dark), Kakureyo (ethereal), combat (rhythm), boss (intensity), victory (short sting). MVP: title, Aokawa, Kakureyo, combat, Hollow Stag and victory cue. Music state machine exploration/combat/boss/transition; crossfade 1–2s, avoid restarting on every enemy aggro, 5s combat exit hysteresis. Optional synchronized stems after baseline; identical sample rate, length and loop markers, sample-accurate loops tested. No voice acting required MVP.

## 11. Ambient and SFX

Ambient layers region/time/weather: birds, river, wind, bamboo, insects, rain, forge, crowd, spectral hum. SFX: surface footsteps, swing, hit, block, parry, dodge, bow, cast, heal, UI confirm/cancel, pickup, quest, craft, shrine, realm transition, boss telegraph. Critical cues distinct even with music disabled. Footsteps surface-aware and cadence-linked; 3–5 variants for frequent effects, modest pitch variation ±5% if appropriate. Spatial 2D panning and attenuation from local listener, no sound across realms except authored transitions. Local speculative swing may play once; confirmed hit audio keyed by event ID to avoid duplicate on reconciliation. Pause freezes gameplay sound events and optionally ducks music; UI/network cues remain active. Accessibility captions for critical audio, mono option, separate volume buses.

## 12. Audio architecture and format

SDL3 audio device/stream output; propose SDL_mixer or custom minimal mixer only after API/build verification at 0.12. No custom codec. Source WAV 48kHz PCM 24-bit preferred; runtime WAV PCM16 for short SFX, Ogg Vorbis for longer tracks/ambient with verified decoder dependency. Mono positional effects; stereo music/ambience. Stream long tracks; preload short effects. Loop points sample-index metadata; check seams and clipping. Buses Master/Music/Ambience/SFX/UI/Voice; default relative gains 100/70/65/85/80/100 percent, user-adjustable. 32 simultaneous voices initial, prioritize combat cues and UI, steal quiet low-priority voices. Master limiter/headroom, target final peaks <= -1 dBFS; music reference around -18 LUFS integrated (creative target, not universal norm). Audio callback must avoid allocation, blocking locks, filesystem I/O. Headless server emits gameplay events, never opens audio device.

## 13. Asset pipeline

Source -> review -> export -> validate -> deterministic pack -> manifest -> client load -> in-game QA. Tools candidates LibreSprite/Aseprite, Tiled, Audacity, LMMS/Reaper or equivalent, Python build scripts; verify versions and licenses at technical architecture. Source art kept editable; runtime assets generated, not manually edited. CI checks dimensions, anchors, alpha, duplicate IDs, missing frames, atlas limits, references, loop bounds, licensing metadata, case-sensitive paths and deterministic hashes. Separate gameplay content hash from cosmetic asset manifest where safe; cosmetic assets cannot alter gameplay collision/timing. Use Git LFS only when needed and configured; avoid committing giant generated duplicates. Asset reload development-only with safe resource lifetime.

```text
assets/
  source/{art,maps,audio}/
  runtime/{atlases,maps,audio,fonts}/
  data/{items,combat,world,ui}/
  manifests/asset_manifest.json
  licenses/
tools/assets/{export,validate,pack}/
src/presentation/{render,animation,ui,vfx,audio}/
```

Naming: lowercase snake_case ASCII, stable logical IDs, no spaces or case-only distinctions. Examples `chr_player_body_south_walk_00.png`, `tile_aokawa_grass_01.png`, `vfx_hollow_slash_00.png`, `sfx_combat_katana_hit_01.wav`, `mus_aokawa_day.ogg`, `amb_shinrin_bamboo_wind.ogg`. Versions in metadata, not references. JSON example:

```json
{
  "id": "chr_player_body_south_walk",
  "atlas": "characters_00",
  "canvas": [16, 32],
  "pivot": [8, 31],
  "loop": true,
  "frames": [{ "rect": [4, 4, 16, 32], "duration_ms": 125 }],
  "license_id": "original_thj"
}
```

## 14. Licensing

Original assets preferred; retain creator/date/source and ownership or contributor agreement. External assets require exact license verification, commercial use/modification/redistribution permissions as applicable, attribution and notices. CC0 generally low-friction; CC BY requires attribution; CC BY-SA requires analysis of adapted asset obligations; avoid NC/ND for commercial/modified use unless separately permitted. Font OFL licenses typically require retaining notices and checking reserved names when modifying. Do not use ripped sprites, game soundtracks, random search images or unlicensed samples. Reference boards are not production assets. Maintain manifest with asset ID, author, source URL, license version, attribution text, modification record, proof of permission and review status. Reject unknown licenses; review separately for code dependencies. AI-generated assets, if used, require provenance and rights review; no guarantee of exclusivity. No external assets are included here.

## 15. Budgets and milestones

Prototype: one player sprite, basic tiles, idle/walk/attack, HUD, placeholder audio, camera, atlas validation. Aokawa slice: village/forest/shrine/dungeon tiles, seven human NPCs + Kuro, three to five enemies and Stag, three weapon visual sets, core effects, six music cues, ~30 SFX families, ambient layers. Multiplayer: cosmetic event dedup, remote animation and weather sync. Expansion: eight regions, additional realm art, outfits, bosses, seasonal art and advanced audio. Target 60 FPS at 480x270 world resolution; 16.67ms frame, initial 96 MiB GPU textures and 32 MiB decoded audio budgets within earlier <512MiB vertical slice process memory target (measure separately); 16 lights, 512 particles, 32 voices. Profile Intel UHD 620/i5-8250U; reduce cosmetic quality before gameplay tick. No benchmark yet.

## 16. Acceptance criteria

- [ ] Pixel-perfect integer scaling and stable camera; correct layer sorting and pivots.
- [ ] Atlas bounds and references validated; no texture seams.
- [ ] Animation timing consistent at varied render FPS and pause.
- [ ] Combat cues readable in every lighting/weather state.
- [ ] UI usable by keyboard/gamepad, scalable, non-color-only indicators.
- [ ] Region and realm palettes consistent; no placeholder confusion in release.
- [ ] Audio loops seamless, no clipping or duplicated confirmed events.
- [ ] Independent volume controls, reduced flash/shake, critical cue captions.
- [ ] Dedicated server runs without graphics/audio devices.
- [ ] Asset pipeline reproducible; license records complete.
- [ ] Frame time, texture memory and voice budgets measured on target hardware.

All numerical budgets, palette, tooling candidates and content counts are proposed and require review/playtesting. This is a design document, not implemented art, music or source code.

## Git checkpoint

```bash
git add docs/
git commit -m "docs(gdd): finalize multiplayer and define art direction audio"
```
