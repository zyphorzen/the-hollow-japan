# Step 0.8 — Inventory, Crafting & Economy

Version: 0.8.0 | Status: Draft — awaiting approval

## 1. Baseline

Slot-based inventory with weight, 32 initial backpack slots, +8 per upgrade to 80; nine equipment slots from Step 0.5; personal storage 100 slots; four consumable references and four skill references; eight trade offer slots per participant. Item definition + instance architecture. Seven rarity tiers, upgrade cap +10, durability, six professions, Mon and Spirit Shards. NPC and direct co-op trade; no auction house in initial release, real-money trade or pay-to-win. SQLite persistence and host authority.

Core loop: gather → process → craft → improve → use/trade → reinvest. Combat and professions offer complementary gear paths.

## 2. Inventory architecture and invariants

Definitions are immutable versioned templates: ID, name, description, category, icon, base stats, rarity, stack limit, integer weight in grams, binding and requirements. Instances have stable UUID, definition ID, quantity, actual rarity, durability, upgrade, rolled modifiers, enchantments, owner/container/slot and revision. A stack has its own UUID; splitting creates a new ID, merging retires the empty ID. Nonstackable equipment quantity is one. A live item belongs to exactly one container and slot; quickbar is a reference, not ownership. Wallet is separate from weight. Equipment main/off hand constraints and all nine slots follow Step 0.5. Two-handed equip reserves both hands without duplicating item. Quest items use protected quest storage when capacity would block progression. Overflow rewards enter a persistent claim queue rather than disappearing.

Capacity kg = 30 + 2\*STR + VIT + bonus. Include backpack and equipped items. Normal pickup/trade cannot exceed capacity; stat changes may create overweight. Ratio <0.8 normal, 0.8–1 inclusive heavy (speed x0.9, no sprint), >1 overweight (speed x0.5, no dodge/sprint). Stack limits: equipment 1, consumables/food 20, materials/ammunition 99, quest items definition-dependent. Stack only matching properties. Move/swap/split/merge/equip/unequip/drop/pickup are validated commands; atomic swap, reject invalid capacity without mutation. Storage access at safe facilities; no remote chest access in combat.

## 3. Rarity and equipment

| Rarity    | Base weapon power multiplier | Enchantment slots |
| --------- | ---------------------------: | ----------------: |
| Common    |                         1.00 |                 0 |
| Uncommon  |                         1.08 |                 1 |
| Rare      |                         1.18 |                 1 |
| Epic      |                         1.30 |                 2 |
| Legendary |                         1.45 |                 2 |
| Mythic    |                         1.60 |                 3 |
| Relic     |          Explicit definition |            Custom |

Rarity is not a multiplier on all character stats. Binding: unbound, character-bound, world-bound, quest-bound. World-bound items only trade inside origin world; quest-bound cannot normally sell/drop/destroy. Requirements use level/attributes/mastery, not class. Base power at equal item level: floor(base _ rarity _ (1 + 0.06\*upgrade)); output is W in Step 0.6, never apply rarity/upgrade twice. Broken gear contributes no gear modifiers and cannot attack; unarmed remains available. Armor scaling is configured separately.

## 4. Loot

Conditional equipment rarity weights Common 50, Uncommon 28, Rare 15, Epic 5, Legendary 1.8, Mythic 0.2; filter eligible entries then normalize. Relic is explicit story/unique loot. At 20% equipment drop chance with all entries eligible, Mythic probability is 0.04%. Host uses stable encounter ID, independent RNG stream and recipient ID; persist resolved outcomes before delivery. First-clear and repeat rewards have distinct claim keys. Loot is individual in co-op; eligibility follows contribution policy.

Hollow Stag: 150–250 Mon, 1 Hollow Antler, 2–4 Spirit Essence; Stag Guardian Charm guaranteed on first clear; equipment roll 30%; bonus rare material 15%. Repeat reward cooldown and encounter IDs prevent reload farming. Full bags route eligible rewards to claim queue.

## 5. Upgrade, enchantment and durability

Upgrade deterministic success if all requirements met, no destruction lottery. Example Iron Katana target levels:

| Target | Materials                 |  Mon |
| ------ | ------------------------- | ---: |
| +1     | Iron Ore x2               |  100 |
| +2     | Iron Ore x3               |  200 |
| +3     | Iron Ingot x2             |  350 |
| +4     | Iron Ingot x3             |  500 |
| +5     | Spirit Essence x2         |  750 |
| +6     | Refined Steel x3          | 1000 |
| +7     | Refined Steel x5          | 1500 |
| +8     | Hollow Shard x3           | 2500 |
| +9     | Ancient Relic Fragment x2 | 4000 |
| +10    | Masterwork Core x1        | 6000 |

Enchants: Flamebound (fire), Spirit Infusion (spiritual), Keen Edge (+3 percentage points crit), Iron Will (+5% posture), Moonveil (+8% spiritual resistance), Swiftblade (-5% attack stamina), Hollow Fang (Hollow damage with corruption cost), Restoration (out-of-combat HP regen). Require recipe, station, profession and materials. No duplicate enchant on same item or incompatible group; replacement consumes new materials. Apply stat caps from Steps 0.5–0.7.

Durability: weapon hit -0.1, block -0.3, armor damaging hit -0.2, death -5% max, movement no loss. Fixed-point integer tenths; clamp to zero. At 0 broken; above 0 through 25% gear power x0.8, otherwise x1.0. Standard repair fee ceil(missing durability \* 3) Mon; other items use configured coefficient. Repair at forge, atomic payment. Durability affects gear contribution only, not whole character power.

## 6. Crafting

Recipe registry validates unlock, profession, station, input materials, currency, output capacity/weight and binding. Plan output and consumption together; commit inventory, wallet, profession XP and reward ledger in one transaction. No RNG failure for baseline recipes. Stations: forge, alchemy table, cooking hearth, workbench, spirit altar, farm plot, fishing spot.

| Recipe             | Inputs                             | Output | Profession level |
| ------------------ | ---------------------------------- | ------ | ---------------: |
| Iron Ingot         | Iron Ore x3, Charcoal x1           | 1      |                1 |
| Iron Katana        | Iron Ingot x4, Wood x2             | 1      |                5 |
| Wooden Yumi        | Bamboo x5, Fiber x3                | 1      |                3 |
| Spirit Staff       | Spirit Wood x3, Spirit Essence x2  | 1      |                5 |
| Healing Salve      | Healing Herb x3, Purified Water x1 | 1      |                1 |
| Purification Charm | Moonleaf x2, Spirit Essence x1     | 1      |               10 |
| Onigiri            | Rice x2                            | 1      |                1 |
| Hot Miso Soup      | Soybean x2, Water x1               | 1      |                3 |

Profession XP uses Step 0.7 curve, reduced reward for trivial recipes. Batch crafting validates total input/output and integer overflow. Crafted quality is deterministic by recipe and eligible mastery, not rerollable through reconnect. High-tier recipes are learned from mentors, quests or discovery.

## 7. Gathering, farming, fishing

Gathering nodes persist node ID, realm, remaining stock and respawn tick. Healing Herb (no tool, 1 day), Bamboo (axe, 1 day), Iron Deposit (pickaxe, 3 days), Spirit Wood (spiritual axe, 3 days), Moonleaf (no tool, 2 days), Hollow Crystal (spirit pickaxe, 5 days). Host serializes competing harvests; no negative stock. World clock, not wall time or chunk reload, controls respawn.

Crop lifecycle planted/sprout/growing/mature/harvest. Rice 5 days spring/summer; soybean 4 spring/summer; healing herb 3 all seasons; pumpkin 7 autumn; snow herb 6 winter; Moonleaf 5 Spirit Realm. Unwatered or unsuitable season pauses growth. No offline progression. Plot IDs and ownership persist. Harvest atomic with inventory and XP.

Fishing: cast, bite, hook, tension minigame, catch/escape. River Carp (Aokawa daytime), Koi (pond all day), Mountain Trout (Yukimine daytime), Silver Sardine (Shiohama coast), Moonfin (Kakureyo night), Spirit Eel (rare event). Host owns attempt ID, eligibility, RNG, tension validation and final catch; cancel/disconnect cannot reroll a resolved catch. Bait consumed on valid cast. Prototype fishing/farming after core vertical slice.

## 8. Currency and merchant economy

Mon is integer primary wallet; Spirit Shards are character-bound secondary reward currency, not convertible or transferable. Hollow Shard remains a crafting material distinct from Spirit Shards. Wallet uint64 with checked arithmetic and design cap 1e9 Mon and 1e6 Spirit Shards. Initial Mon 100. Sources: quests, enemy drops, selling goods, bounties. Sinks: purchases, repair, crafting fees, upgrading, enchantment, respec, housing, fast travel (optional post-MVP). Player trade transfers Mon without creating it; merchant payouts mint and purchases burn in the abstract economy.

Base price P in Mon. Buy from merchant = ceil(P*1.25*D*R); sell to merchant = floor(P*0.50*D*Q), where D regional demand 0.8–1.2, R reputation 0.85–1.15, Q condition/quality factor 0–1 for ordinary resale. Base value already includes equipment rarity and upgrade valuation. Sell price may be zero (merchant declines); no free purchases. Fixed-point pricing and checked integer math. Demand shared for buy and sell, bounded daily adjustments, buyback at original paid-to-player amount subject to limited time/stock; exclude buyback from arbitrary price modifiers. For identical condition/base value, worst ordinary sell 0.60P < cheapest buy 0.85P, preventing simple cross-region arbitrage; intentional trade routes use separately balanced commodities. Merchant has stock and buy budget, refreshed on world day; no unlimited vendor purchases. Price quotes carry revision and expiry; stale quote rejected. Merchant stock and wallet changes atomic.

Example base values: Healing Herb 5, Iron Ore 12, Healing Salve 30, Iron Katana 250. NPCs: Yuki general goods, Takeshi gear and repair, Hana alchemy, Sora fish and bait. Reputation discounts applied once. Quest-bound and character-bound items cannot be sold unless explicitly marked for permitted disposal. Sell/buyback preserves exact instance and binding.

Economy targets (provisional): early income 150–300 Mon per 30-minute session, repair/consumables 15–30% of income, optional upgrades absorb surplus; measure median and p90 wallet, net mint/burn, prices, item creation/destruction, material bottlenecks, vendor arbitrage and craft profitability. Test recipes for profitable infinite loops including XP and resale; no forced daily login. Death currency recovery follows Step 0.4: transfer 10% carried Mon to cache, no new currency minted; separate transaction invalidates previous cache. Durability penalty is additional proposed cost.

## 9. Co-op trade and anti-duplication

Direct trade in same realm, safe area, within 48 px, both alive/out of combat. Up to 8 item offers each plus Mon. Spirit Shards and bound items restricted by binding rules. Open → offer → both confirm same revision → validate → atomic commit → complete; changing offer clears both confirmations. Cancel, disconnect, movement away or timeout releases locks. Items remain in owned inventory with reservation; no clone into trade UI. Validate ownership, quantity, balance, slot and weight after both sides' outgoing items, binding, offer revision and session identity. No partial transfer, no negative balances. Trade fee 0 in MVP.

All mutations host-authoritative; client sends intention and expected revision, not outcomes. Serialize inventory commands on authoritative simulation/transaction boundary. SQLite transaction with constraints on UUID and owner/container/slot; command ledger unique (world, authenticated character, command ID), payload digest and stored result; duplicate exact request returns original result, same ID/different payload rejected. IDs stable across reconnect. Atomic inventory/wallet/reward/profession update plus durable commit before acknowledgement; failure rolls back. On crash recover last committed state; event outbox/revisions allow resend and snapshot reconciliation. No simultaneous split/drop/trade of reserved items. Loot claims unique (world, encounter/claim ID, recipient). Validate capacity and checked arithmetic. Cross-world character import remains pending Step 0.10; untrusted offline saves cannot mint goods into authoritative world. Host cheating cannot be fully prevented in peer-hosted model; server-managed economies would require separate trusted authority.

## 10. Architecture

`ItemRegistry`, `InventorySystem`, `EquipmentSystem`, `LootSystem`, `CraftingSystem`, `UpgradeService`, `EnchantmentSystem`, `DurabilitySystem`, `GatheringSystem`, `FarmingSystem`, `FishingSystem`, `WalletService`, `MerchantSystem`, `TradeService`, `EconomyLedger`. Simulation independent of SDL renderer. Data JSON under assets/data/items, recipes, loot_tables, merchants, resources, crops, fish, enchants. SQLite tables: item_instances, containers, inventory_slots, wallets, resource_nodes, farm_plots, merchant_stock, merchant_budgets, trade_sessions, command_ledger, reward_claims, pending_rewards, economy_events. Stable IDs, schema version, foreign keys, transactional mutations. No need to create all files before implementation.

## 11. Milestones and acceptance criteria

Prototype: inventory add/remove/split/merge, equipment, wallet, loot and persistence. Aokawa: basic recipes, gathering, forge upgrade/repair, merchants, individual loot, overflow claims. Co-op alpha: trade, reservation, command replay, crash recovery, authoritative loot and concurrent gathering. Expansion: farming, fishing, enchantments, high-tier economy and balancing.

Tests: stack conservation, unique ownership, capacity after swap, two-hand equip, protected quest items, loot determinism and eligibility, no repeat first-clear reward, insufficient inputs rollback, batch overflow checks, upgrade cap, enchant compatibility, broken gear effects, persistent resource respawn, crop growth, fishing attempt replay, wallet conservation in trade, stale quote/offer rejection, reconnect replay, crash before/after commit, simultaneous commands, no negative quantities, vendor arbitrage and crafting loop audit. All implementation tests pending.

## Git checkpoint

```bash
git add docs/
git commit -m "docs(gdd): finalize progression and define inventory crafting economy"
```

Decisions: slot and weight capacity, currencies, rarity, upgrade fees, durability, recipes, merchant margins, trade rules and economic targets are proposed. No C++ implementation, benchmark or playtesting yet. Static document excludes interactive UI previews.
