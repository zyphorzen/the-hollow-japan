# The Hollow Japan — GDD v1.0.0

> Beyond the living world, something waits.

## Dokumentasi

- [01 / Game Identity & Vision](gdd/01-game-identity.md) — Finalized
- [02 / Lore & Storyline](gdd/02-lore-and-story.md) — Finalized
- [03 / World geography](gdd/03-world-building.md) — Finalized
- [Step 0.4 — Core Gameplay Loop](gdd/04-core-gameplay-loop.md) — Finalized
- [Step 0.5 — Player & Character System](gdd/05-player-character-system.md) — Finalized
- [Step 0.6 — Hybrid Combat System](gdd/06-hybrid-combat-system.md) — Finalized
- [Step 0.7 — Progression & Skill System](gdd/07-progression-skill-system.md) — Finalized
- [Step 0.8 — Inventory, Crafting & Economy](gdd/08-inventory-crafting-economy.md) — Finalized
- [Step 0.9 — NPC & Quest System](gdd/09-npc-quest-system.md) — Finalized
- [Step 0.10 — Multiplayer Design](gdd/10-multiplayer-design.md) — Finalized
- [Step 0.11 — Art Direction & Audio](gdd/11-art-direction-audio.md) — Finalized
- [Step 0.12 — Technical Architecture](gdd/12-technical-architecture.md) — Finalized
- [Step 0.13 — MVP & Development Scope](gdd/13-mvp-development-scope.md) — Finalized

## Approved baseline

All 13 chapters approved on 2026-11-26. Step 0.13 governs delivery scope. Singleplayer MVP first, LAN alpha next, online co-op thereafter. Numerical balancing, dependency pins, effort and performance targets remain subject to validation. Historical chapter versions identify their original design revisions. No engine implementation or benchmark is claimed.

## Implementation

- [Step 1.1 — Development Environment Verification](development/01-foundation/01-environment-verification.md) — Pending local verification.

## Git checkpoint

```bash
git add README.md docs/
git diff --cached --stat
git commit -m "docs(gdd): approve pre-production baseline v1.0.0"
git tag -a gdd-v1.0.0 -m "Approved Game Design Document"
```
