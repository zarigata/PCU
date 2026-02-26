# VoxelForge Modding API

```
██╗   ██╗ ██████╗ ██╗██████╗ ███████╗██████╗ 
██║   ██║██╔═══██╗██║██╔══██╗██╔════╝██╔══██╗
██║   ██║██║   ██║██║██║  ██║█████╗  ██████╔╝
╚██╗ ██╔╝██║   ██║██║██║  ██║██╔══╝  ██╔══██╗
 ╚████╔╝ ╚██████╔╝██║██████╔╝███████╗██║  ██║
  ╚═══╝   ╚═════╝ ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝
              Modding System Documentation
```

## Overview

VoxelForge features a **dual-layer modding system**:
1. **Native C++ Plugins** - Maximum performance, full engine access
2. **Lua Scripts** - Safe sandboxing, rapid development, hot reloading

Both systems share the same underlying API through a unified interface.

---

## Table of Contents

1. [Mod Structure](#mod-structure)
2. [Mod Manifest](#mod-manifest)
3. [Native C++ API](#native-c-api)
4. [Lua Scripting API](#lua-scripting-api)
5. [Registry System](#registry-system)
6. [Event System](#event-system)
7. [Best Practices](#best-practices)

---

## 1. Mod Structure

### Directory Layout

```
mods/
├── my_mod/
│   ├── mod.json              # Required: Mod manifest
│   ├── libmy_mod.so          # Optional: Native plugin (Linux)
│   ├── libmy_mod.dll         # Optional: Native plugin (Windows)
│   ├── libmy_mod.dylib       # Optional: Native plugin (macOS)
│   ├── scripts/              # Optional: Lua scripts
│   │   ├── init.lua          # Entry point
│   │   ├── blocks.lua
│   │   ├── items.lua
│   │   └── events.lua
│   ├── assets/               # Optional: Mod resources
│   │   ├── textures/
│   │   │   └── block/
│   │   │       └── my_block.png
│   │   ├── models/
│   │   │   └── block/
│   │   │       └── my_block.json
│   │   ├── sounds/
│   │   └── lang/
│   │       └── en_us.json
│   ├── data/                 # Optional: Data files
│   │   ├── recipes/
│   │   ├── loot_tables/
│   │   └── structures/
│   └── config/               # Optional: Default config
│       └── my_mod.json
```

---

## 2. Mod Manifest

### mod.json Schema

```json
{
    "id": "my_mod",
    "version": "1.0.0",
    "name": "My Awesome Mod",
    "description": "Adds awesome features to VoxelForge",
    "authors": ["DeveloperName"],
    "contributors": ["HelperName"],
    "license": "MIT",
    "website": "https://example.com/my_mod",
    "repository": "https://github.com/user/my_mod",
    "issues": "https://github.com/user/my_mod/issues",
    
    "voxelforge_version": ">=1.0.0 <2.0.0",
    "minecraft_version": "1.20.1",
    
    "dependencies": {
        "core": ">=1.0.0",
        "other_mod": "^2.0.0",
        "optional_mod": "~1.5.0"
    },
    
    "incompatibilities": {
        "conflicting_mod": "*"
    },
    
    "entrypoints": {
        "native": "libmy_mod",
        "script": "scripts/init.lua",
        "client": "scripts/client.lua",
        "server": "scripts/server.lua"
    },
    
    "mixins": [
        "mixins/my_mod.mixins.json"
    ],
    
    "resources": {
        "textures": "assets/textures",
        "models": "assets/models",
        "sounds": "assets/sounds",
        "shaders": "assets/shaders",
        "lang": "assets/lang"
    },
    
    "data": {
        "recipes": "data/recipes",
        "loot_tables": "data/loot_tables",
        "structures": "data/structures",
        "advancements": "data/advancements",
        "tags": "data/tags"
    },
    
    "config": {
        "file": "config/my_mod.json",
        "schema": "config/schema.json"
    },
    
    "side": "both",
    "load_order": "normal",
    "required_on_client": false,
    "required_on_server": false
}
```

### Version Constraints

| Syntax | Meaning |
|--------|---------|
| `1.0.0` | Exact version |
| `>=1.0.0` | Version 1.0.0 or higher |
| `<2.0.0` | Version below 2.0.0 |
| `^1.2.3` | Compatible (1.2.3 to <2.0.0) |
| `~1.2.3` | Approximately (1.2.3 to <1.3.0) |
| `*` | Any version |

### Side Values

| Value | Description |
|-------|-------------|
| `both` | Required on both client and server |
| `client` | Client-side only |
| `server` | Server-side only |

---

## 3. Native C++ API

### Plugin Entry Points

```cpp
// mod_entry.cpp

#include <VoxelForge/Modding/ModAPI.h>

// Required: Mod information
extern "C" VOXELFORGE_API ModInfo mod_get_info() {
    return ModInfo{
        .id = "my_mod",
        .version = "1.0.0",
        .name = "My Mod",
        .description = "A native plugin example",
        .author = "Developer"
    };
}

// Required: Initialization
extern "C" VOXELFORGE_API void mod_load(ModContext* ctx) {
    // Get logger
    auto& logger = ctx->getLogger();
    logger.info("My Mod loading...");
    
    // Register content
    registerBlocks(ctx);
    registerItems(ctx);
    registerEntities(ctx);
    registerEvents(ctx);
    registerCommands(ctx);
    
    logger.info("My Mod loaded successfully!");
}

// Required: Cleanup
extern "C" VOXELFORGE_API void mod_unload() {
    // Cleanup resources
}
```

### Block Registration

```cpp
void registerBlocks(ModContext* ctx) {
    auto& registry = ctx->getRegistry<BlockRegistry>();
    
    // Simple block
    BlockID myBlock = registry.registerBlock("my_mod:custom_block", BlockDefinition{
        .id = "my_mod:custom_block",
        .name = "Custom Block",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 15.0f,
        .renderType = RenderType::Solid,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .lightEmission = 7,
        .sounds = BlockSoundGroup::STONE,
        .collisionShape = VoxelShape::fullCube()
    });
    
    // Block with custom behavior
    registry.registerBlock("my_mod:interactive_block", BlockDefinition{
        .id = "my_mod:interactive_block",
        .name = "Interactive Block",
        .material = Material::Wood,
        .hardness = 2.0f,
        .hasBlockEntity = true
    }, std::make_unique<InteractiveBlockBehavior>());
}

// Custom block behavior
class InteractiveBlockBehavior : public IBlockBehavior {
public:
    void onUse(World& world, Player& player, const BlockPos& pos, BlockState state) override {
        player.sendMessage("You interacted with the block!");
        
        // Open GUI
        if (auto* be = world.getBlockEntity(pos)) {
            player.openGui(be->getGuiId());
        }
    }
    
    void onRandomTick(World& world, const BlockPos& pos, BlockState state) override {
        // Random tick behavior
        if (rand() % 100 == 0) {
            world.setBlock(pos, state.withProperty("age", state.getInt("age") + 1));
        }
    }
    
    VoxelShape getCollisionShape(BlockState state) const override {
        // Variable collision based on state
        int fill = state.getInt("fill");
        return VoxelShape::cube(0, 0, 0, 16, fill, 16);
    }
};
```

### Item Registration

```cpp
void registerItems(ModContext* ctx) {
    auto& registry = ctx->getRegistry<ItemRegistry>();
    
    // Simple item
    registry.registerItem("my_mod:custom_item", ItemDefinition{
        .id = "my_mod:custom_item",
        .name = "Custom Item",
        .maxStackSize = 64,
        .rarity = Rarity::Rare,
        .category = ItemCategory::Misc
    });
    
    // Item with custom behavior
    registry.registerItem("my_mod:magic_wand", ItemDefinition{
        .id = "my_mod:magic_wand",
        .name = "Magic Wand",
        .maxStackSize = 1,
        .rarity = Rarity::Epic,
        .enchantability = 30
    }, std::make_unique<MagicWandBehavior>());
    
    // Food item
    registry.registerItem("my_mod:custom_food", ItemDefinition{
        .id = "my_mod:custom_food",
        .name = "Tasty Treat",
        .maxStackSize = 64,
        .foodProperties = FoodProperties{
            .nutrition = 6,
            .saturation = 0.8f,
            .isMeat = false,
            .fast = false,
            .alwaysEdible = true
        }
    });
    
    // Tool item
    registry.registerTool("my_mod:custom_pickaxe", ToolDefinition{
        .id = "my_mod:custom_pickaxe",
        .name = "Custom Pickaxe",
        .toolType = ToolType::Pickaxe,
        .tier = CustomToolTier{
            .level = 4,
            .uses = 2000,
            .speed = 10.0f,
            .attackDamage = 5.0f,
            .attackSpeed = -2.8f,
            .enchantability = 20
        }
    });
    
    // Armor item
    registry.registerArmor("my_mod:custom_helmet", ArmorDefinition{
        .id = "my_mod:custom_helmet",
        .name = "Custom Helmet",
        .slot = ArmorSlot::Head,
        .defense = 3,
        .toughness = 2.0f,
        .knockbackResistance = 0.1f,
        .material = CustomArmorMaterial{
            .durabilityMultiplier = 40,
            .defense = {3, 6, 8, 3},
            .enchantability = 20,
            .toughness = 2.0f,
            .knockbackResistance = 0.1f,
            .repairItem = "my_mod:custom_gem"
        }
    });
}

// Custom item behavior
class MagicWandBehavior : public IItemBehavior {
public:
    bool onUse(ItemUseContext& ctx) override {
        World& world = ctx.getWorld();
        Player& player = ctx.getPlayer();
        Vec3 lookDir = player.getLookDirection();
        
        // Spawn particle trail
        for (int i = 0; i < 10; i++) {
            Vec3 pos = player.getPosition() + lookDir * (i * 0.5f);
            world.spawnParticle(ParticleType::Spell, pos, {0, 0.1f, 0}, 0.5f);
        }
        
        // Cast spell
        RaycastResult hit = world.raycast(player.getPosition(), lookDir, 50.0f);
        if (hit.hit) {
            world.createExplosion(hit.position, 2.0f, false, true);
        }
        
        // Damage item
        ctx.getItemStack().damage(1, player);
        
        return true;
    }
    
    bool onEntityHit(ItemHitEntityContext& ctx) override {
        Entity& target = ctx.getTarget();
        target.damage(10.0f, DamageSource::magic(ctx.getPlayer()));
        return true;
    }
};
```

### Entity Registration

```cpp
void registerEntities(ModContext* ctx) {
    auto& registry = ctx->getRegistry<EntityRegistry>();
    
    // Custom mob
    registry.registerEntity("my_mod:custom_mob", EntityDefinition{
        .id = "my_mod:custom_mob",
        .name = "Custom Mob",
        .type = EntityType::Mob,
        .category = EntityCategory::Monster,
        .spawnGroup = SpawnGroup::Monster,
        .spawnWeight = 100,
        .minSpawnSize = 1,
        .maxSpawnSize = 3,
        .spawnRules = SpawnRules{
            .dimensions = {"minecraft:overworld"},
            .biomes = {"*"},  // All biomes
            .lightLevel = {0, 7},
            .blockRequirement = {"minecraft:stone", "minecraft:grass"}
        }
    }, std::make_unique<CustomMobFactory>());
    
    // Custom projectile
    registry.registerEntity("my_mod:magic_projectile", EntityDefinition{
        .id = "my_mod:magic_projectile",
        .name = "Magic Projectile",
        .type = EntityType::Projectile,
        .category = EntityCategory::Misc,
        .trackingRange = 64,
        .updateInterval = 1
    }, std::make_unique<MagicProjectileFactory>());
}

// Custom mob implementation
class CustomMob : public MobEntity {
public:
    CustomMob(World& world) : MobEntity(world) {
        // Set attributes
        setMaxHealth(50.0f);
        setHealth(50.0f);
        setAttackDamage(8.0f);
        setMovementSpeed(0.3f);
        setFollowRange(32.0f);
        
        // Setup AI
        setupAI();
    }
    
    void setupAI() {
        auto& brain = getBrain();
        
        // Goal selector
        brain.addGoal(0, std::make_unique<SwimGoal>(*this));
        brain.addGoal(1, std::make_unique<MeleeAttackGoal>(*this, 1.0, true));
        brain.addGoal(2, std::make_unique<MoveTowardsTargetGoal>(*this, 1.0, 32.0f));
        brain.addGoal(3, std::make_unique<WanderAroundGoal>(*this, 0.8));
        brain.addGoal(4, std::make_unique<LookAtEntityGoal>(*this, EntityType::Player, 8.0f));
        
        // Target selector
        brain.addTargetGoal(1, std::make_unique<RevengeGoal>(*this));
        brain.addTargetGoal(2, std::make_unique<ActiveTargetGoal<Player>>(*this, true));
    }
    
    void tick() override {
        MobEntity::tick();
        
        // Custom tick behavior
        if (getTarget() && rand() % 100 == 0) {
            // Special attack
            performSpecialAttack();
        }
    }
    
    void performSpecialAttack() {
        // Spawn particles
        for (int i = 0; i < 20; i++) {
            Vec3 offset = Vec3(
                (rand() % 100 - 50) / 50.0f,
                (rand() % 100) / 50.0f,
                (rand() % 100 - 50) / 50.0f
            );
            world.spawnParticle(ParticleType::Spell, getPosition() + offset, {0, 0.1f, 0}, 1.0f);
        }
        
        // Area damage
        auto entities = world.getEntitiesInRange(getPosition(), 5.0f);
        for (auto& entity : entities) {
            if (entity != this && entity->isAlive()) {
                entity->damage(5.0f, DamageSource::magic(this));
            }
        }
    }
};
```

### Event Handling

```cpp
void registerEvents(ModContext* ctx) {
    auto& bus = ctx->getEventBus();
    
    // Block events
    bus.subscribe<BlockPlaceEvent>([](const BlockPlaceEvent& e) {
        // Cancel if player doesn't have permission
        if (e.player && !e.player->hasPermission("build")) {
            e.cancel();
            e.player->sendMessage("You don't have permission to build here!");
            return;
        }
        
        // Log block placement
        auto& logger = ModContext::get("my_mod")->getLogger();
        logger.info("{} placed {} at ({}, {}, {})", 
            e.player->getName(),
            e.block.getDefinition().name,
            e.position.x, e.position.y, e.position.z
        );
    });
    
    // Entity events
    bus.subscribe<EntityDamageEvent>([](const EntityDamageEvent& e) {
        // Custom damage reduction
        if (e.entity->hasComponent<CustomArmorComponent>()) {
            float reduction = e.entity->getComponent<CustomArmorComponent>()->damageReduction;
            e.setDamage(e.getDamage() * (1.0f - reduction));
        }
    });
    
    // Player events
    bus.subscribe<PlayerJoinEvent>([](const PlayerJoinEvent& e) {
        e.player->sendMessage("Welcome! My Mod is active on this server.");
        
        // Give starting items to new players
        if (!e.player->hasPlayedBefore()) {
            e.player->getInventory().addItem(ItemStack("my_mod:starter_kit", 1));
        }
    });
    
    // Tick events
    bus.subscribe<WorldTickEvent>([](const WorldTickEvent& e) {
        // Every 20 ticks (1 second)
        if (e.world->getTickCounter() % 20 == 0) {
            // Do periodic tasks
        }
    });
    
    // Custom events
    bus.subscribe<CustomSpellCastEvent>([](const CustomSpellCastEvent& e) {
        // Handle custom mod event
        e.caster->addEffect(StatusEffect(EffectType::Levitation, 5 * 20, 1));
    });
}
```

### Command Registration

```cpp
void registerCommands(ModContext* ctx) {
    auto& registry = ctx->getCommandRegistry();
    
    // Simple command
    registry.registerCommand("heal", [](CommandContext& ctx) {
        Player* player = ctx.getPlayer();
        if (!player) {
            ctx.sendError("This command can only be used by players!");
            return;
        }
        
        player->setHealth(player->getMaxHealth());
        player->sendMessage("You have been healed!");
        ctx.sendSuccess("Healed " + player->getName());
    })
    .setPermission("my_mod.heal")
    .setDescription("Heals the player to full health")
    .setUsage("/heal")
    .registerAlias("health");
    
    // Complex command with arguments
    registry.registerCommand("spawnmob")
        .argument<EntityType>("mob_type")
        .argument<Position>("position", true)  // Optional
        .argument<Integer>("count", true)
        .execute([](CommandContext& ctx) {
            EntityType mobType = ctx.getArgument<EntityType>("mob_type");
            Position pos = ctx.getArgumentOrDefault<Position>("position", 
                ctx.getPlayer()->getPosition());
            int count = ctx.getArgumentOrDefault<Integer>("count", 1);
            
            World& world = ctx.getWorld();
            for (int i = 0; i < count; i++) {
                Vec3 spawnPos = pos + Vec3(
                    (rand() % 20 - 10) / 10.0f,
                    0,
                    (rand() % 20 - 10) / 10.0f
                );
                world.spawnEntity(mobType, spawnPos);
            }
            
            ctx.sendSuccess("Spawned " + std::to_string(count) + " " + mobType.name);
        })
        .setPermission("my_mod.spawnmob")
        .setDescription("Spawns mobs at a position")
        .setUsage("/spawnmob <mob_type> [position] [count]");
    
    // Command with subcommands
    registry.registerCommand("my_mod")
        .subcommand("reload", [](CommandContext& ctx) {
            ctx.getModContext()->reload();
            ctx.sendSuccess("My Mod reloaded!");
        })
        .subcommand("info", [](CommandContext& ctx) {
            ctx.sendInfo("My Mod v1.0.0");
            ctx.sendInfo("Author: Developer");
        })
        .subcommand("config")
            .argument<String>("key")
            .argument<String>("value", true)
            .execute([](CommandContext& ctx) {
                String key = ctx.getArgument<String>("key");
                if (ctx.hasArgument("value")) {
                    String value = ctx.getArgument<String>("value");
                    ctx.getModContext()->setConfig(key, value);
                    ctx.sendSuccess("Set " + key + " to " + value);
                } else {
                    String value = ctx.getModContext()->getConfig(key);
                    ctx.sendInfo(key + " = " + value);
                }
            });
}
```

---

## 4. Lua Scripting API

### Entry Point (init.lua)

```lua
-- init.lua
-- Mod entry point

local my_mod = {}

-- Called when mod loads
function my_mod.load(ctx)
    my_mod.ctx = ctx
    my_mod.logger = ctx:getLogger()
    
    my_mod.logger:info("My Mod loading from Lua!")
    
    -- Register content
    my_mod.registerBlocks()
    my_mod.registerItems()
    my_mod.registerEntities()
    my_mod.registerEvents()
    my_mod.registerCommands()
    
    my_mod.logger:info("My Mod loaded successfully!")
end

-- Called when mod unloads
function my_mod.unload()
    my_mod.logger:info("My Mod unloading...")
end

return my_mod
```

### Block Registration (Lua)

```lua
-- scripts/blocks.lua

function my_mod.registerBlocks()
    local blockRegistry = my_mod.ctx:getRegistry("block")
    
    -- Simple block
    blockRegistry:register("my_mod:custom_block", {
        name = "Custom Block",
        material = "stone",
        hardness = 3.0,
        blast_resistance = 15.0,
        render_type = "solid",
        required_tool = "pickaxe",
        light_level = 7,
        
        sounds = {
            break = "block.stone.break",
            step = "block.stone.step",
            place = "block.stone.place",
            hit = "block.stone.hit"
        }
    })
    
    -- Block with properties
    blockRegistry:register("my_mod:directional_block", {
        name = "Directional Block",
        material = "wood",
        hardness = 2.0,
        
        properties = {
            facing = {"north", "south", "east", "west"},
            powered = {true, false}
        },
        
        default_state = {
            facing = "north",
            powered = false
        },
        
        -- Block behaviors
        on_place = function(pos, state, placer, context)
            -- Face toward placer
            local look_dir = placer:getLookDirection()
            local facing = look_dir.x > 0 and "east" or
                          look_dir.x < 0 and "west" or
                          look_dir.z > 0 and "south" or "north"
            return state:with("facing", facing)
        end,
        
        on_use = function(pos, state, player, hand)
            -- Toggle powered state
            local powered = state:get("powered")
            local new_state = state:with("powered", not powered)
            my_mod.ctx:getWorld():setBlock(pos, new_state)
            return true
        end,
        
        on_neighbor_update = function(pos, state, neighbor_pos, neighbor_state)
            -- React to redstone
            local world = my_mod.ctx:getWorld()
            local power = world:getRedstonePower(pos)
            
            if state:get("powered") ~= (power > 0) then
                return state:with("powered", power > 0)
            end
            return state
        end
    })
    
    -- Crop block
    blockRegistry:register("my_mod:custom_crop", {
        name = "Custom Crop",
        material = "plant",
        render_type = "cutout",
        
        properties = {
            age = {min = 0, max = 7}
        },
        
        on_random_tick = function(pos, state, world, random)
            -- Growth
            local age = state:get("age")
            if age < 7 and random:nextFloat() < 0.1 then
                world:setBlock(pos, state:with("age", age + 1))
            end
        end,
        
        get_drops = function(pos, state, breaker, tool)
            local age = state:get("age")
            if age == 7 then
                return {
                    {item = "my_mod:custom_seed", count = random:int(1, 3)},
                    {item = "my_mod:custom_produce", count = 1}
                }
            else
                return {{item = "my_mod:custom_seed", count = 1}}
            end
        end
    })
end
```

### Item Registration (Lua)

```lua
-- scripts/items.lua

function my_mod.registerItems()
    local itemRegistry = my_mod.ctx:getRegistry("item")
    
    -- Simple item
    itemRegistry:register("my_mod:custom_item", {
        name = "Custom Item",
        max_stack = 64,
        rarity = "rare",
        category = "misc"
    })
    
    -- Food item
    itemRegistry:register("my_mod:custom_food", {
        name = "Tasty Treat",
        max_stack = 64,
        
        food = {
            nutrition = 6,
            saturation = 0.8,
            is_meat = false,
            fast = false,
            always_edible = true
        },
        
        on_eaten = function(stack, player, world)
            player:addEffect("regeneration", 10 * 20, 1)
            player:addEffect("speed", 30 * 20, 0)
            return stack:shrink(1)
        end
    })
    
    -- Tool item
    itemRegistry:register("my_mod:custom_sword", {
        name = "Custom Sword",
        max_stack = 1,
        
        tool = {
            type = "sword",
            tier = {
                level = 4,
                uses = 2000,
                speed = 10.0,
                damage = 8.0,
                enchantability = 20
            }
        },
        
        on_hit_entity = function(stack, target, attacker)
            -- Apply special effect on hit
            target:addEffect("slowness", 5 * 20, 2)
            
            -- 10% chance to deal bonus damage
            if math.random() < 0.1 then
                target:damage(5, attacker)
                attacker:sendMessage("Critical strike!")
            end
            
            return stack:damage(1, attacker)
        end
    })
    
    -- Armor item
    itemRegistry:register("my_mod:custom_chestplate", {
        name = "Custom Chestplate",
        max_stack = 1,
        
        armor = {
            slot = "chest",
            defense = 8,
            toughness = 3.0,
            knockback_resistance = 0.15,
            material = {
                durability_multiplier = 40,
                enchantability = 20,
                repair_item = "my_mod:custom_gem"
            }
        },
        
        on_equipped = function(stack, player)
            player:addEffect("resistance", -1, 0)  -- Permanent while equipped
        end,
        
        on_unequipped = function(stack, player)
            player:removeEffect("resistance")
        end
    })
end
```

### Entity Registration (Lua)

```lua
-- scripts/entities.lua

function my_mod.registerEntities()
    local entityRegistry = my_mod.ctx:getRegistry("entity")
    
    -- Custom mob
    entityRegistry:register("my_mod:custom_mob", {
        name = "Custom Mob",
        type = "mob",
        category = "monster",
        
        attributes = {
            max_health = 50,
            attack_damage = 8,
            movement_speed = 0.3,
            follow_range = 32
        },
        
        dimensions = {
            width = 0.9,
            height = 1.8
        },
        
        spawn = {
            weight = 100,
            min_group = 1,
            max_group = 3,
            dimensions = {"minecraft:overworld"},
            biomes = {"*"},
            light = {min = 0, max = 7}
        },
        
        -- AI goals
        goals = {
            {priority = 0, type = "swim"},
            {priority = 1, type = "melee_attack", speed = 1.0, memory = 20},
            {priority = 2, type = "move_to_target", speed = 1.0, range = 32},
            {priority = 3, type = "wander", speed = 0.8},
            {priority = 4, type = "look_at_player", range = 8}
        },
        
        targets = {
            {priority = 1, type = "revenge"},
            {priority = 2, type = "nearest_player", visibility = true}
        },
        
        -- Drops
        drops = {
            {item = "my_mod:custom_item", min = 1, max = 3, chance = 0.5},
            {item = "my_mod:rare_drop", min = 1, max = 1, chance = 0.05}
        },
        
        -- Experience
        experience = 10,
        
        -- Custom behaviors
        tick = function(self, world)
            -- Custom tick behavior
            if self.target and math.random() < 0.01 then
                self:specialAttack()
            end
        end,
        
        special_attack = function(self)
            local world = self:getWorld()
            
            -- Particle effect
            for i = 1, 20 do
                local offset = vec3(
                    (math.random() - 0.5) * 2,
                    math.random() * 2,
                    (math.random() - 0.5) * 2
                )
                world:spawnParticle("spell", self.position + offset, vec3(0, 0.1, 0))
            end
            
            -- Area damage
            local entities = world:getEntitiesInRange(self.position, 5)
            for _, entity in ipairs(entities) do
                if entity ~= self and entity:isAlive() then
                    entity:damage(5, self)
                end
            end
        end
    })
    
    -- Custom projectile
    entityRegistry:register("my_mod:magic_projectile", {
        name = "Magic Projectile",
        type = "projectile",
        
        projectile = {
            gravity = 0.0,
            drag = 0.01,
            max_lifetime = 100,
            piercing = true,
            max_pierce = 3
        },
        
        on_hit_entity = function(self, entity)
            entity:damage(10, self.owner)
            entity:addEffect("slowness", 5 * 20, 1)
            self:discard()
        end,
        
        on_hit_block = function(self, pos, state)
            local world = self:getWorld()
            world:spawnParticle("explosion", pos, vec3(0, 0, 0))
            self:discard()
        end,
        
        tick = function(self, world)
            -- Trail particles
            world:spawnParticle("end_rod", self.position, vec3(0, 0, 0))
        end
    })
end
```

### Event Handling (Lua)

```lua
-- scripts/events.lua

function my_mod.registerEvents()
    local eventBus = my_mod.ctx:getEventBus()
    
    -- Block events
    eventBus:on("block_place", function(e)
        -- Log placement
        my_mod.logger:info(string.format("%s placed %s at %s",
            e.player:getName(),
            e.block:getName(),
            e.position:toString()
        ))
        
        -- Check permission
        if not e.player:hasPermission("my_mod.build") then
            e:cancel()
            e.player:sendMessage("You don't have permission!")
        end
    end)
    
    eventBus:on("block_break", function(e)
        -- Bonus drops for lucky players
        if e.player and math.random() < 0.1 then
            local world = my_mod.ctx:getWorld()
            world:spawnItem(e.position, e.block:getDrops()[1].item, 1)
            e.player:sendMessage("Lucky bonus drop!")
        end
    end)
    
    -- Entity events
    eventBus:on("entity_damage", function(e)
        -- Damage reduction for players with custom armor
        if e.entity:isPlayer() then
            local armor = e.entity:getArmor("chest")
            if armor and armor:getItem() == "my_mod:custom_chestplate" then
                e:setDamage(e:getDamage() * 0.8)
            end
        end
    end)
    
    eventBus:on("entity_death", function(e)
        -- Custom death messages
        if e.entity:isPlayer() then
            local world = e.entity:getWorld()
            world:broadcast(string.format("§c%s was slain!", e.entity:getName()))
        end
    end)
    
    -- Player events
    eventBus:on("player_join", function(e)
        -- Welcome message
        e.player:sendMessage("§aWelcome to the server! My Mod is active.")
        
        -- First join kit
        if not e.player:hasPlayedBefore() then
            local inv = e.player:getInventory()
            inv:addItem(ItemStack("my_mod:starter_kit", 1))
            e.player:sendMessage("§eYou received a starter kit!")
        end
    end)
    
    eventBus:on("player_leave", function(e)
        my_mod.logger:info(e.player:getName() .. " left the server")
    end)
    
    -- World events
    eventBus:on("world_tick", function(e)
        -- Every second (20 ticks)
        if e.world:getTickCount() % 20 == 0 then
            -- Periodic task
            my_mod.doPeriodicTask(e.world)
        end
    end)
    
    eventBus:on("chunk_load", function(e)
        -- Custom chunk generation
        if my_mod.config.generate_structures then
            my_mod.tryGenerateStructure(e.chunk)
        end
    end)
    
    -- Item events
    eventBus:on("item_crafted", function(e)
        -- Bonus for crafting custom items
        if e.result:getItem():startsWith("my_mod:") then
            e.player:addExperience(10)
        end
    end)
    
    -- Command events
    eventBus:on("command_execute", function(e)
        my_mod.logger:info(string.format("%s executed: %s",
            e.sender:getName(),
            e.command
        ))
    end)
end
```

### Command Registration (Lua)

```lua
-- scripts/commands.lua

function my_mod.registerCommands()
    local cmdRegistry = my_mod.ctx:getCommandRegistry()
    
    -- Simple command
    cmdRegistry:register("heal", function(ctx)
        local player = ctx:getPlayer()
        if not player then
            ctx:sendError("Only players can use this command!")
            return
        end
        
        player:setHealth(player:getMaxHealth())
        player:sendMessage("§aYou have been healed!")
        ctx:sendSuccess("Healed " .. player:getName())
    end)
    :setPermission("my_mod.heal")
    :setDescription("Heals the player to full health")
    :setUsage("/heal")
    :addAlias("health")
    
    -- Command with arguments
    cmdRegistry:register("spawnmob")
        :argument("entity", "mob_type")
        :argument("position", "pos", true)  -- Optional
        :argument("integer", "count", true)
        :execute(function(ctx)
            local mobType = ctx:getArgument("mob_type")
            local pos = ctx:getArgumentOr("pos", ctx:getPlayer():getPosition())
            local count = ctx:getArgumentOr("count", 1)
            
            local world = ctx:getWorld()
            for i = 1, count do
                local spawnPos = pos + vec3(
                    (math.random() - 0.5) * 2,
                    0,
                    (math.random() - 0.5) * 2
                )
                world:spawnEntity(mobType, spawnPos)
            end
            
            ctx:sendSuccess(string.format("Spawned %d %s", count, mobType))
        end)
        :setPermission("my_mod.spawnmob")
        :setDescription("Spawns mobs")
        :setUsage("/spawnmob <mob_type> [pos] [count]")
    
    -- Command with subcommands
    cmdRegistry:register("mymod")
        :subcommand("reload", function(ctx)
            my_mod.ctx:reload()
            ctx:sendSuccess("My Mod reloaded!")
        end)
        :subcommand("info", function(ctx)
            ctx:sendInfo("My Mod v1.0.0")
            ctx:sendInfo("Author: Developer")
        end)
        :subcommand("config")
            :argument("string", "key")
            :argument("string", "value", true)
            :execute(function(ctx)
                local key = ctx:getArgument("key")
                local value = ctx:getArgumentOr("value", nil)
                
                if value then
                    my_mod.config[key] = value
                    ctx:sendSuccess(string.format("Set %s to %s", key, value))
                else
                    ctx:sendInfo(string.format("%s = %s", key, my_mod.config[key] or "nil"))
                end
            end)
end
```

---

## 5. Registry System

### Available Registries

| Registry | Purpose | Access |
|----------|---------|--------|
| `BlockRegistry` | Block types | `ctx:getRegistry("block")` |
| `ItemRegistry` | Item types | `ctx:getRegistry("item")` |
| `EntityRegistry` | Entity types | `ctx:getRegistry("entity")` |
| `BiomeRegistry` | Biome types | `ctx:getRegistry("biome")` |
| `DimensionRegistry` | Dimensions | `ctx:getRegistry("dimension")` |
| `RecipeRegistry` | Crafting recipes | `ctx:getRegistry("recipe")` |
| `LootTableRegistry` | Loot tables | `ctx:getRegistry("loot_table")` |
| `SoundRegistry` | Sound events | `ctx:getRegistry("sound")` |
| `ParticleRegistry` | Particle types | `ctx:getRegistry("particle")` |
| `EnchantmentRegistry` | Enchantments | `ctx:getRegistry("enchantment")` |
| `PotionRegistry` | Potion effects | `ctx:getRegistry("potion")` |
| `CommandRegistry` | Commands | `ctx:getCommandRegistry()` |

### Recipe Registration

```lua
-- Shaped recipe
recipeRegistry:registerShaped("my_mod:custom_block", {
    pattern = {
        "###",
        "#X#",
        "###"
    },
    keys = {
        ["#"] = "my_mod:custom_gem",
        ["X"] = "minecraft:diamond"
    },
    result = {
        item = "my_mod:custom_block",
        count = 1
    }
})

-- Shapeless recipe
recipeRegistry:registerShapeless("my_mod:custom_dust", {
    ingredients = {
        "my_mod:custom_gem",
        "minecraft:flint"
    },
    result = {
        item = "my_mod:custom_dust",
        count = 4
    }
})

-- Smelting recipe
recipeRegistry:registerSmelting("my_mod:custom_ingot", {
    ingredient = "my_mod:custom_ore",
    result = "my_mod:custom_ingot",
    experience = 1.0,
    cook_time = 200
})

-- Smithing recipe
recipeRegistry:registerSmithing("my_mod:upgraded_sword", {
    base = "my_mod:custom_sword",
    addition = "my_mod:upgrade_gem",
    result = "my_mod:upgraded_sword"
})
```

---

## 6. Event System

### Available Events

| Event | When Triggered | Cancelable |
|-------|----------------|------------|
| `block_place` | Block placed | ✅ |
| `block_break` | Block broken | ✅ |
| `block_use` | Block right-clicked | ✅ |
| `entity_spawn` | Entity spawned | ✅ |
| `entity_death` | Entity dies | ❌ |
| `entity_damage` | Entity damaged | ✅ |
| `player_join` | Player joins server | ❌ |
| `player_leave` | Player leaves server | ❌ |
| `player_chat` | Player sends message | ✅ |
| `player_command` | Player executes command | ✅ |
| `item_craft` | Item crafted | ✅ |
| `item_use` | Item used | ✅ |
| `chunk_load` | Chunk loaded | ❌ |
| `chunk_unload` | Chunk unloaded | ❌ |
| `world_load` | World loaded | ❌ |
| `world_save` | World saved | ❌ |
| `world_tick` | Every game tick | ❌ |
| `explosion` | Explosion occurs | ✅ |

---

## 7. Best Practices

### Performance

1. **Cache registry lookups** - Don't look up the same block/item every tick
2. **Use batch operations** - Group multiple block changes
3. **Avoid heavy operations in tick** - Move to async tasks
4. **Profile your mod** - Use the built-in profiler

### Safety

1. **Always null-check** - Entities can despawn, blocks can change
2. **Validate user input** - Commands, configs, NBT data
3. **Handle exceptions** - Don't crash the game
4. **Test thoroughly** - Edge cases, multiplayer, servers

### Compatibility

1. **Use namespaced IDs** - `my_mod:block_name`, not just `block_name`
2. **Check for conflicts** - Don't override vanilla without need
3. **Document your API** - If other mods might extend yours
4. **Version your config** - Allow config migrations

---

*"With great modding power comes great responsibility."*

**- T-800**
