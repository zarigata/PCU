/**
 * @file CommandManager.cpp
 * @brief Command system implementation
 */

#include <VoxelForge/game/CommandManager.hpp>
#include <VoxelForge/game/Item.hpp>
#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace VoxelForge {

// ============================================================================
// CommandContext Implementation
// ============================================================================

void CommandContext::sendSuccess(const std::string& message) {
    VF_INFO("[Command] {}", message);
}

void CommandContext::sendError(const std::string& message) {
    VF_ERROR("[Command] {}", message);
}

void CommandContext::sendFeedback(const std::string& message) {
    VF_INFO("[Command] {}", message);
}

// ============================================================================
// Command Implementation
// ============================================================================

std::optional<int> Command::parseInteger(const std::string& arg, int min, int max) const {
    try {
        int value = std::stoi(arg);
        if (value < min || value > max) {
            return std::nullopt;
        }
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<float> Command::parseFloat(const std::string& arg, float min, float max) const {
    try {
        float value = std::stof(arg);
        if (value < min || value > max) {
            return std::nullopt;
        }
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> Command::parseBool(const std::string& arg) const {
    std::string lower = arg;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
        return true;
    }
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
        return false;
    }
    return std::nullopt;
}

std::optional<glm::ivec3> Command::parseBlockPos(const std::string& arg) const {
    // Format: x,y,z or x y z or ~ ~ ~
    std::string s = arg;
    std::replace(s.begin(), s.end(), ',', ' ');
    std::istringstream iss(s);
    
    int x, y, z;
    if (!(iss >> x >> y >> z)) {
        return std::nullopt;
    }
    return glm::ivec3(x, y, z);
}

std::optional<glm::vec3> Command::parseVec3(const std::string& arg) const {
    std::string s = arg;
    std::replace(s.begin(), s.end(), ',', ' ');
    std::istringstream iss(s);
    
    float x, y, z;
    if (!(iss >> x >> y >> z)) {
        return std::nullopt;
    }
    return glm::vec3(x, y, z);
}

std::vector<uint32_t> Command::parseSelector(const std::string& arg, CommandContext& ctx) const {
    std::vector<uint32_t> result;
    
    // @p - nearest player
    // @a - all players
    // @e - all entities
    // @r - random player
    // @s - self
    
    if (arg == "@s" || arg == "@p") {
        if (ctx.executorId != 0) {
            result.push_back(ctx.executorId);
        }
    } else if (arg == "@a") {
        // TODO: Get all players from server
    } else if (arg == "@e") {
        // TODO: Get all entities from entity manager
    } else if (arg == "@r") {
        // TODO: Get random player
    } else {
        // Try to parse as player name or entity ID
        try {
            result.push_back(std::stoul(arg));
        } catch (...) {
            // TODO: Look up player by name
        }
    }
    
    return result;
}

// ============================================================================
// CommandManager Implementation
// ============================================================================

CommandManager::CommandManager() {
    VF_INFO("CommandManager created");
}

CommandManager::~CommandManager() {
    VF_INFO("CommandManager destroyed");
}

void CommandManager::registerCommand(std::unique_ptr<Command> command) {
    if (!command || command->name.empty()) {
        VF_ERROR("Cannot register null or empty command");
        return;
    }
    
    std::string name = command->name;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    
    // Register aliases
    for (const auto& alias : command->aliases) {
        std::string lowerAlias = alias;
        std::transform(lowerAlias.begin(), lowerAlias.end(), lowerAlias.begin(), ::tolower);
        aliases[lowerAlias] = name;
    }
    
    commands[name] = std::move(command);
    VF_TRACE("Registered command: {}", name);
}

void CommandManager::unregisterCommand(const std::string& name) {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    // Remove aliases
    std::vector<std::string> aliasesToRemove;
    for (const auto& [alias, cmd] : aliases) {
        if (cmd == lowerName) {
            aliasesToRemove.push_back(alias);
        }
    }
    for (const auto& alias : aliasesToRemove) {
        aliases.erase(alias);
    }
    
    commands.erase(lowerName);
    VF_TRACE("Unregistered command: {}", name);
}

void CommandManager::registerAlias(const std::string& command, const std::string& alias) {
    std::string lowerCmd = command;
    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(), ::tolower);
    std::string lowerAlias = alias;
    std::transform(lowerAlias.begin(), lowerAlias.end(), lowerAlias.begin(), ::tolower);
    
    aliases[lowerAlias] = lowerCmd;
    VF_TRACE("Registered alias '{}' for command '{}'", alias, command);
}

void CommandManager::clear() {
    commands.clear();
    aliases.clear();
    VF_INFO("CommandManager cleared");
}

CommandResult CommandManager::execute(const std::string& input, CommandContext& context) {
    auto tokens = tokenize(input);
    if (tokens.empty()) {
        return {false, "Empty command", 0};
    }
    
    std::string cmdName = tokens[0];
    std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::tolower);
    
    Command* cmd = findCommand(cmdName);
    if (!cmd) {
        return {false, "Unknown command: " + cmdName, 0};
    }
    
    // Check permission
    if (cmd->requiredPermission > context.permissionLevel) {
        return {false, "You don't have permission to use this command", 0};
    }
    
    // Remove command name from tokens
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    
    // Execute
    try {
        return cmd->execute(context, args);
    } catch (const std::exception& e) {
        return {false, std::string("Error executing command: ") + e.what(), 0};
    }
}

CommandResult CommandManager::executeAs(const std::string& input, uint32_t playerId) {
    CommandContext context;
    context.source = CommandSource::Player;
    context.executorId = playerId;
    // TODO: Set other context fields
    
    return execute(input, context);
}

CommandResult CommandManager::executeConsole(const std::string& input) {
    CommandContext context;
    context.source = CommandSource::Console;
    context.permissionLevel = 4; // Console has full permissions
    
    return execute(input, context);
}

std::vector<std::string> CommandManager::tabComplete(const std::string& input, CommandContext& context) {
    std::vector<std::string> completions;
    auto tokens = tokenize(input);
    
    if (tokens.empty()) {
        // Complete command names
        for (const auto& [name, cmd] : commands) {
            completions.push_back(name);
        }
    } else if (tokens.size() == 1) {
        // Complete partial command name
        std::string partial = tokens[0];
        std::transform(partial.begin(), partial.end(), partial.begin(), ::tolower);
        
        for (const auto& [name, cmd] : commands) {
            if (name.find(partial) == 0) {
                completions.push_back(name);
            }
        }
        for (const auto& [alias, cmd] : aliases) {
            if (alias.find(partial) == 0) {
                completions.push_back(alias);
            }
        }
    } else {
        // Complete command arguments
        Command* cmd = findCommand(tokens[0]);
        if (cmd && cmd->tabComplete) {
            completions = cmd->tabComplete(tokens.back());
        }
    }
    
    return completions;
}

const Command* CommandManager::getCommand(const std::string& name) const {
    return findCommand(name);
}

bool CommandManager::hasCommand(const std::string& name) const {
    return findCommand(name) != nullptr;
}

std::vector<const Command*> CommandManager::getAllCommands() const {
    std::vector<const Command*> result;
    for (const auto& [name, cmd] : commands) {
        result.push_back(cmd.get());
    }
    return result;
}

std::vector<const Command*> CommandManager::getCommandsForPermission(int level) const {
    std::vector<const Command*> result;
    for (const auto& [name, cmd] : commands) {
        if (cmd->requiredPermission <= level) {
            result.push_back(cmd.get());
        }
    }
    return result;
}

void CommandManager::registerVanillaCommands() {
    VF_INFO("Registering vanilla commands...");
    
    registerCommand(Commands::createHelpCommand());
    registerCommand(Commands::createGamemodeCommand());
    registerCommand(Commands::createTeleportCommand());
    registerCommand(Commands::createGiveCommand());
    registerCommand(Commands::createTimeCommand());
    registerCommand(Commands::createWeatherCommand());
    registerCommand(Commands::createKillCommand());
    registerCommand(Commands::createHealCommand());
    registerCommand(Commands::createClearCommand());
    registerCommand(Commands::createListCommand());
    registerCommand(Commands::createStopCommand());
    registerCommand(Commands::createSayCommand());
    registerCommand(Commands::createDifficultyCommand());
    registerCommand(Commands::createSeedCommand());
    registerCommand(Commands::createWorldBorderCommand());
    registerCommand(Commands::createSummonCommand());
    registerCommand(Commands::createSetBlockCommand());
    
    VF_INFO("Registered {} commands", commands.size());
}

std::vector<std::string> CommandManager::tokenize(const std::string& input) const {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    
    for (char c : input) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (std::isspace(c) && !inQuotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

std::string CommandManager::joinArguments(const std::vector<std::string>& args, size_t start) const {
    std::string result;
    for (size_t i = start; i < args.size(); ++i) {
        if (i > start) result += " ";
        result += args[i];
    }
    return result;
}

Command* CommandManager::findCommand(const std::string& name) {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    auto it = commands.find(lowerName);
    if (it != commands.end()) {
        return it->second.get();
    }
    
    // Check aliases
    auto aliasIt = aliases.find(lowerName);
    if (aliasIt != aliases.end()) {
        auto cmdIt = commands.find(aliasIt->second);
        if (cmdIt != commands.end()) {
            return cmdIt->second.get();
        }
    }
    
    return nullptr;
}

const Command* CommandManager::findCommand(const std::string& name) const {
    return const_cast<CommandManager*>(this)->findCommand(name);
}

// ============================================================================
// Built-in Commands Implementation
// ============================================================================

namespace Commands {

std::unique_ptr<Command> createHelpCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "help";
    cmd->description = "Shows help for commands";
    cmd->usage = "help [command]";
    cmd->aliases = {"?"};
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        ctx.sendSuccess("Available commands: /help, /gamemode, /tp, /give, /time, /weather, /kill, /heal, /clear, /list, /stop, /summon, /setblock");
        return {true, "", 0};
    };
    return cmd;
}

std::unique_ptr<Command> createGamemodeCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "gamemode";
    cmd->description = "Changes game mode";
    cmd->usage = "gamemode <mode> [player]";
    cmd->aliases = {"gm"};
    cmd->requiredPermission = 2;
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        if (args.empty()) {
            return {false, "Usage: gamemode <mode> [player]", 0};
        }
        ctx.sendSuccess("Game mode changed to " + args[0]);
        return {true, "", 1};
    };
    return cmd;
}

std::unique_ptr<Command> createTeleportCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "tp";
    cmd->description = "Teleports entities";
    cmd->usage = "tp <destination> OR tp <targets> <destination>";
    cmd->aliases = {"teleport"};
    cmd->requiredPermission = 2;
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        if (args.empty()) {
            return {false, "Usage: tp <x> <y> <z>", 0};
        }
        ctx.sendSuccess("Teleported to " + args[0]);
        return {true, "", 1};
    };
    return cmd;
}

std::unique_ptr<Command> createGiveCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "give";
    cmd->description = "Gives items to players";
    cmd->usage = "give <player> <item> [count]";
    cmd->requiredPermission = 2;
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        if (args.size() < 2) {
            return {false, "Usage: give <player> <item> [count]", 0};
        }
        int count = args.size() > 2 ? std::stoi(args[2]) : 1;
        ctx.sendSuccess("Gave " + args[1] + " x" + std::to_string(count) + " to " + args[0]);
        return {true, "", count};
    };
    return cmd;
}

std::unique_ptr<Command> createTimeCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "time";
    cmd->description = "Changes or queries time";
    cmd->usage = "time <set|add|query> <value>";
    cmd->requiredPermission = 2;
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        if (args.empty()) {
            return {false, "Usage: time <set|add|query> <value>", 0};
        }
        ctx.sendSuccess("Time " + args[0] + " " + (args.size() > 1 ? args[1] : ""));
        return {true, "", 1};
    };
    return cmd;
}

std::unique_ptr<Command> createWeatherCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "weather";
    cmd->description = "Sets the weather";
    cmd->usage = "weather <clear|rain|thunder> [duration]";
    cmd->requiredPermission = 2;
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        if (args.empty()) {
            return {false, "Usage: weather <clear|rain|thunder> [duration]", 0};
        }
        ctx.sendSuccess("Weather set to " + args[0]);
        return {true, "", 1};
    };
    return cmd;
}

std::unique_ptr<Command> createKillCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "kill";
    cmd->description = "Kills entities";
    cmd->usage = "kill [targets]";
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        ctx.sendSuccess("Killed target");
        return {true, "", 1};
    };
    return cmd;
}

std::unique_ptr<Command> createHealCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "heal";
    cmd->description = "Heals entities";
    cmd->usage = "heal [targets] [amount]";
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        ctx.sendSuccess("Healed target");
        return {true, "", 1};
    };
    return cmd;
}

std::unique_ptr<Command> createClearCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "clear";
    cmd->description = "Clears inventory";
    cmd->usage = "clear [player] [item] [maxCount]";
    cmd->requiredPermission = 2;
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        ctx.sendSuccess("Cleared inventory");
        return {true, "", 1};
    };
    return cmd;
}

std::unique_ptr<Command> createListCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "list";
    cmd->description = "Lists players";
    cmd->usage = "list";
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        ctx.sendSuccess("Players online: 0");
        return {true, "", 0};
    };
    return cmd;
}

std::unique_ptr<Command> createStopCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "stop";
    cmd->description = "Stops the server";
    cmd->usage = "stop";
    cmd->requiredPermission = 4;
    cmd->operatorOnly = true;
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        ctx.sendSuccess("Stopping server...");
        return {true, "", 1};
    };
    return cmd;
}

std::unique_ptr<Command> createSayCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "say";
    cmd->description = "Broadcasts a message";
    cmd->usage = "say <message>";
    cmd->requiredPermission = 2;
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        if (args.empty()) {
            return {false, "Usage: say <message>", 0};
        }
        std::string msg;
        for (const auto& arg : args) {
            if (!msg.empty()) msg += " ";
            msg += arg;
        }
        ctx.sendSuccess("[Server] " + msg);
        return {true, "", 1};
    };
    return cmd;
}

std::unique_ptr<Command> createDifficultyCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "difficulty";
    cmd->description = "Sets the difficulty";
    cmd->usage = "difficulty <peaceful|easy|normal|hard>";
    cmd->requiredPermission = 2;
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        if (args.empty()) {
            return {false, "Usage: difficulty <peaceful|easy|normal|hard>", 0};
        }
        ctx.sendSuccess("Difficulty set to " + args[0]);
        return {true, "", 1};
    };
    return cmd;
}

std::unique_ptr<Command> createSeedCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "seed";
    cmd->description = "Shows the world seed";
    cmd->usage = "seed";
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        ctx.sendSuccess("Seed: 123456789");
        return {true, "", 0};
    };
    return cmd;
}

// Placeholder implementations for remaining commands
std::unique_ptr<Command> createEffectCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createEnchantCommand() { return std::make_unique<Command>(); }

std::unique_ptr<Command> createSummonCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "summon";
    cmd->description = "Summons an entity";
    cmd->usage = "summon <entity> [<x> <y> <z>] [nbt]";
    cmd->requiredPermission = 2;
    cmd->arguments = {
        {"entity", ArgumentType::String, false, {}, ""},
        {"x", ArgumentType::Float, true, {}, "~"},
        {"y", ArgumentType::Float, true, {}, "~"},
        {"z", ArgumentType::Float, true, {}, "~"},
        {"nbt", ArgumentType::String, true, {}, ""}
    };

    // List of known entity IDs and their factory mappings
    static const std::unordered_map<std::string, std::string> entityIdAliases = {
        {"zombie", "poorcraftultra:zombie"},
        {"skeleton", "poorcraftultra:skeleton"},
        {"creeper", "poorcraftultra:creeper"},
        {"spider", "poorcraftultra:spider"},
        {"enderman", "poorcraftultra:enderman"},
        {"blaze", "poorcraftultra:blaze"},
        {"slime", "poorcraftultra:slime"},
        {"cow", "poorcraftultra:cow"},
        {"pig", "poorcraftultra:pig"},
        {"sheep", "poorcraftultra:sheep"},
        {"chicken", "poorcraftultra:chicken"},
        {"arrow", "poorcraftultra:arrow"},
        {"snowball", "poorcraftultra:snowball"},
        {"item", "poorcraftultra:item"},
    };

    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        if (args.empty()) {
            return {false, "Usage: summon <entity> [<x> <y> <z>] [nbt]", 0};
        }

        // Resolve entity type
        std::string entityType = args[0];
        // Lowercase for comparison
        std::string entityTypeLower = entityType;
        std::transform(entityTypeLower.begin(), entityTypeLower.end(), entityTypeLower.begin(), ::tolower);

        // Strip namespace prefix for matching if it matches ours
        std::string bare = entityTypeLower;
        const std::string ns = "poorcraftultra:";
        if (bare.rfind(ns, 0) == 0) {
            bare = bare.substr(ns.size());
        }
        // Also handle minecraft: prefix
        const std::string mcns = "minecraft:";
        if (bare.rfind(mcns, 0) == 0) {
            bare = bare.substr(mcns.size());
        }

        // Parse position
        glm::vec3 spawnPos = ctx.position; // Default: executor's position
        bool hasPos = false;

        if (args.size() >= 4) {
            // Try to parse x y z
            auto px = args[1];
            auto py = args[2];
            auto pz = args[3];

            // Handle ~ relative notation
            auto parseCoord = [&](const std::string& s, float base) -> std::optional<float> {
                if (s.empty()) return std::nullopt;
                if (s[0] == '~') {
                    if (s.size() == 1) return base; // Just ~ means use base
                    try {
                        return base + std::stof(s.substr(1));
                    } catch (...) {
                        return std::nullopt;
                    }
                }
                try {
                    return std::stof(s);
                } catch (...) {
                    return std::nullopt;
                }
            };

            auto ox = parseCoord(px, spawnPos.x);
            auto oy = parseCoord(py, spawnPos.y);
            auto oz = parseCoord(pz, spawnPos.z);

            if (!ox || !oy || !oz) {
                return {false, "Invalid position coordinates", 0};
            }

            spawnPos = glm::vec3(*ox, *oy, *oz);
            hasPos = true;
        }

        // Spawn the entity using the ECS world
        // The CommandContext may have an ECS world through the server/world.
        // For now, use EntityFactory which requires an ECSWorld.
        // We'll log the spawn and return success; the actual spawn will be
        // routed through the world's entity manager when integrated.

        EntityID spawnedEntity = INVALID_ENTITY;

        // Determine entity type and create via factory
        if (ctx.world) {
            auto& ecsWorld = ctx.world->getECSWorld();

            if (bare == "zombie") {
                spawnedEntity = EntityFactory::createZombie(ecsWorld, spawnPos);
            } else if (bare == "skeleton") {
                spawnedEntity = EntityFactory::createSkeleton(ecsWorld, spawnPos);
            } else if (bare == "creeper") {
                spawnedEntity = EntityFactory::createCreeper(ecsWorld, spawnPos);
            } else if (bare == "spider") {
                spawnedEntity = MobFactory::createSpider(ecsWorld, spawnPos);
            } else if (bare == "enderman") {
                spawnedEntity = MobFactory::createEnderman(ecsWorld, spawnPos);
            } else if (bare == "blaze") {
                spawnedEntity = MobFactory::createBlaze(ecsWorld, spawnPos);
            } else if (bare == "slime") {
                spawnedEntity = MobFactory::createSlime(ecsWorld, spawnPos, 1);
            } else if (bare == "cow") {
                spawnedEntity = EntityFactory::createCow(ecsWorld, spawnPos);
            } else if (bare == "pig") {
                spawnedEntity = EntityFactory::createPig(ecsWorld, spawnPos);
            } else if (bare == "sheep") {
                spawnedEntity = EntityFactory::createSheep(ecsWorld, spawnPos);
            } else if (bare == "chicken") {
                spawnedEntity = EntityFactory::createChicken(ecsWorld, spawnPos);
            } else if (bare == "arrow") {
                auto vel = glm::vec3(0.0f, 0.0f, 0.0f);
                spawnedEntity = EntityFactory::createArrow(ecsWorld, spawnPos, vel, INVALID_ENTITY);
            } else if (bare == "snowball") {
                auto vel = glm::vec3(0.0f, 0.0f, 0.0f);
                spawnedEntity = EntityFactory::createSnowball(ecsWorld, spawnPos, vel, INVALID_ENTITY);
            } else if (bare == "item") {
                // Summoning an item entity requires an item stack; create empty for now
                ItemStack emptyStack;
                spawnedEntity = EntityFactory::createItem(ecsWorld, spawnPos, emptyStack);
            } else {
                // Generic entity spawn for unknown types
                spawnedEntity = ecsWorld.createEntity();
                auto& transform = ecsWorld.addComponent<TransformComponent>(spawnedEntity);
                transform.position = spawnPos;
                auto& name = ecsWorld.addComponent<NameComponent>(spawnedEntity);
                name.name = bare;
                auto& base = ecsWorld.addComponent<EntityBaseComponent>(spawnedEntity);
                base.type = EntityType::Generic;
                base.uuid = UUID::generate();
            }
        }

        if (spawnedEntity == INVALID_ENTITY) {
            return {false, "Failed to summon entity: " + entityType, 0};
        }

        // Format position string
        std::string posStr = hasPos ?
            (std::to_string(static_cast<int>(spawnPos.x)) + ", " +
             std::to_string(static_cast<int>(spawnPos.y)) + ", " +
             std::to_string(static_cast<int>(spawnPos.z))) :
            "executor position";

        ctx.sendSuccess("Summoned " + entityType + " at (" + posStr + ")");
        return {true, "", 1};
    };

    // Tab completion: suggest entity types
    cmd->tabComplete = [](const std::string& partial) -> std::vector<std::string> {
        static const std::vector<std::string> entityTypes = {
            "zombie", "skeleton", "creeper", "spider", "enderman", "blaze", "slime",
            "cow", "pig", "sheep", "chicken", "arrow", "snowball", "item"
        };
        std::vector<std::string> results;
        std::string lower = partial;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& type : entityTypes) {
            if (type.find(lower) == 0) {
                results.push_back(type);
            }
        }
        return results;
    };

    return cmd;
}

std::unique_ptr<Command> createSetBlockCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "setblock";
    cmd->description = "Changes a block to another block";
    cmd->usage = "setblock <x> <y> <z> <block> [destroy|keep|replace]";
    cmd->requiredPermission = 2;
    cmd->arguments = {
        {"x", ArgumentType::Integer, false, {}, "~"},
        {"y", ArgumentType::Integer, false, {}, "~"},
        {"z", ArgumentType::Integer, false, {}, "~"},
        {"block", ArgumentType::String, false, {}, ""},
        {"mode", ArgumentType::String, true, {"destroy", "keep", "replace"}, "replace"}
    };

    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        if (args.size() < 4) {
            return {false, "Usage: setblock <x> <y> <z> <block> [destroy|keep|replace]", 0};
        }

        if (!ctx.world) {
            return {false, "No world context available", 0};
        }

        // Parse coordinates with ~ relative support
        auto parseCoord = [](const std::string& s, int base) -> std::optional<int> {
            if (s.empty()) return std::nullopt;
            if (s[0] == '~') {
                if (s.size() == 1) return base;
                try { return base + std::stoi(s.substr(1)); } catch (...) { return std::nullopt; }
            }
            try { return std::stoi(s); } catch (...) { return std::nullopt; }
        };

        auto ox = parseCoord(args[0], static_cast<int>(ctx.position.x));
        auto oy = parseCoord(args[1], static_cast<int>(ctx.position.y));
        auto oz = parseCoord(args[2], static_cast<int>(ctx.position.z));

        if (!ox || !oy || !oz) {
            return {false, "Invalid position coordinates", 0};
        }

        int bx = *ox, by = *oy, bz = *oz;

        // Resolve block name to BlockID
        std::string blockName = args[3];
        // Add minecraft: prefix if no namespace given
        if (blockName.find(':') == std::string::npos) {
            blockName = "minecraft:" + blockName;
        }

        BlockID blockId = BlockRegistry::get().getBlockId(blockName);
        if (blockId == AIR_BLOCK && blockName != "minecraft:air") {
            // Unknown block — return error
            return {false, "Unknown block: " + args[3], 0};
        }

        BlockState newState(blockId);

        // Parse mode
        std::string mode = "replace";
        if (args.size() >= 5) {
            mode = args[4];
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
        }

        BlockState existing = ctx.world->getBlock(bx, by, bz);

        if (mode == "keep") {
            // Only place if the existing block is air
            if (!existing.isAir()) {
                ctx.sendSuccess("Block not placed (keep mode: non-air block at position)");
                return {true, "", 0};
            }
        } else if (mode == "destroy") {
            // Drop the existing block's loot before replacing
            if (!existing.isAir()) {
                VF_INFO("[setblock] Destroying block {} at ({}, {}, {})",
                         static_cast<int>(existing.getBlockId()), bx, by, bz);
            }
        }
        // mode == "replace" or fallthrough from destroy/keep: set the block

        ctx.world->setBlock(bx, by, bz, newState);

        ctx.sendSuccess("Block set to " + args[3] +
            " at (" + std::to_string(bx) + ", " + std::to_string(by) + ", " + std::to_string(bz) + ")");
        return {true, "", 1};
    };

    // Tab completion for block names and modes
    cmd->tabComplete = [](const std::string& partial) -> std::vector<std::string> {
        static const std::vector<std::string> modes = {"destroy", "keep", "replace"};
        std::vector<std::string> results;
        std::string lower = partial;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& m : modes) {
            if (m.find(lower) == 0) results.push_back(m);
        }
        return results;
    };

    return cmd;
}
std::unique_ptr<Command> createFillCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createCloneCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createOpCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createDeopCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createKickCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createBanCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createWhitelistCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createTellCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createTitleCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createScoreboardCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createWorldBorderCommand() {
    auto cmd = std::make_unique<Command>();
    cmd->name = "worldborder";
    cmd->description = "Manages the world border";
    cmd->usage = "worldborder <set|center|damage|warning|get|add> ...";
    cmd->aliases = {"wb"};
    cmd->requiredPermission = 2;
    cmd->execute = [](CommandContext& ctx, const std::vector<std::string>& args) -> CommandResult {
        if (args.empty()) {
            return {false, "Usage: worldborder <set|center|damage|warning|get|add> ...", 0};
        }

        if (!ctx.world) {
            return {false, "No world context available", 0};
        }

        auto& border = ctx.world->getWorldBorder();
        const auto& sub = args[0];

        if (sub == "set") {
            if (args.size() < 2) {
                return {false, "Usage: worldborder set <size> [timeSeconds]", 0};
            }
            try {
                double size = std::stod(args[1]);
                if (args.size() >= 3) {
                    int64_t seconds = std::stoll(args[2]);
                    border.interpolateSize(size, seconds * 1000);
                    ctx.sendSuccess("World border transitioning to " + args[1] + " over " + args[2] + "s");
                } else {
                    border.setSize(size);
                    ctx.sendSuccess("World border set to " + args[1]);
                }
                return {true, "", 1};
            } catch (...) {
                return {false, "Invalid size value", 0};
            }
        }

        if (sub == "center") {
            if (args.size() < 3) {
                return {false, "Usage: worldborder center <x> <z>", 0};
            }
            try {
                double x = std::stod(args[1]);
                double z = std::stod(args[2]);
                border.setCenter(x, z);
                ctx.sendSuccess("World border center set to " + args[1] + ", " + args[2]);
                return {true, "", 1};
            } catch (...) {
                return {false, "Invalid center coordinates", 0};
            }
        }

        if (sub == "add") {
            if (args.size() < 2) {
                return {false, "Usage: worldborder add <amount> [timeSeconds]", 0};
            }
            try {
                double amount = std::stod(args[1]);
                double target = border.getSize() + amount;
                if (args.size() >= 3) {
                    int64_t seconds = std::stoll(args[2]);
                    border.interpolateSize(target, seconds * 1000);
                    ctx.sendSuccess("World border growing by " + args[1] + " over " + args[2] + "s");
                } else {
                    border.setSize(target);
                    ctx.sendSuccess("World border grown by " + args[1]);
                }
                return {true, "", 1};
            } catch (...) {
                return {false, "Invalid amount", 0};
            }
        }

        if (sub == "damage") {
            if (args.size() < 2) {
                return {false, "Usage: worldborder damage <perBlock|buffer> <value>", 0};
            }
            if (args[1] == "perBlock" && args.size() >= 3) {
                try {
                    float dps = std::stof(args[2]);
                    border.setDamagePerSecond(dps);
                    ctx.sendSuccess("World border damage set to " + args[2] + " per second");
                    return {true, "", 1};
                } catch (...) {
                    return {false, "Invalid damage value", 0};
                }
            }
            if (args[1] == "buffer" && args.size() >= 3) {
                try {
                    int blocks = std::stoi(args[2]);
                    border.setSafeZoneBlocks(blocks);
                    ctx.sendSuccess("World border safe zone set to " + args[2] + " blocks");
                    return {true, "", 1};
                } catch (...) {
                    return {false, "Invalid buffer value", 0};
                }
            }
            return {false, "Usage: worldborder damage <perBlock|buffer> <value>", 0};
        }

        if (sub == "warning") {
            if (args.size() < 2) {
                return {false, "Usage: worldborder warning <distance|time> <value>", 0};
            }
            if (args[1] == "distance" && args.size() >= 3) {
                try {
                    int blocks = std::stoi(args[2]);
                    border.setWarningDistance(blocks);
                    ctx.sendSuccess("World border warning distance set to " + args[2] + " blocks");
                    return {true, "", 1};
                } catch (...) {
                    return {false, "Invalid distance", 0};
                }
            }
            if (args[1] == "time" && args.size() >= 3) {
                try {
                    int seconds = std::stoi(args[2]);
                    border.setWarningTimeSeconds(seconds);
                    ctx.sendSuccess("World border warning time set to " + args[2] + "s");
                    return {true, "", 1};
                } catch (...) {
                    return {false, "Invalid time", 0};
                }
            }
            return {false, "Usage: worldborder warning <distance|time> <value>", 0};
        }

        if (sub == "get") {
            ctx.sendSuccess("World border: size=" + std::to_string(border.getSize()) +
                " center=(" + std::to_string(border.getCenterX()) + "," + std::to_string(border.getCenterZ()) + ")" +
                " damage=" + std::to_string(border.getDamagePerSecond()) + "/s");
            return {true, "", 1};
        }

        return {false, "Unknown subcommand. Use: set|center|add|damage|warning|get", 0};
    };
    return cmd;
}
std::unique_ptr<Command> createLocateCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createSpreadPlayersCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createParticleCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createPlaySoundCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createXPCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createAttributeCommand() { return std::make_unique<Command>(); }

} // namespace Commands

} // namespace VoxelForge
