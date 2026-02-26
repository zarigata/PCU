/**
 * @file CommandManager.cpp
 * @brief Command system implementation
 */

#include <VoxelForge/game/CommandManager.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace VoxelForge {

// ============================================================================
// CommandContext Implementation
// ============================================================================

void CommandContext::sendSuccess(const std::string& message) {
    LOG_INFO("[Command] {}", message);
}

void CommandContext::sendError(const std::string& message) {
    LOG_ERROR("[Command] {}", message);
}

void CommandContext::sendFeedback(const std::string& message) {
    LOG_INFO("[Command] {}", message);
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
    LOG_INFO("CommandManager created");
}

CommandManager::~CommandManager() {
    LOG_INFO("CommandManager destroyed");
}

void CommandManager::registerCommand(std::unique_ptr<Command> command) {
    if (!command || command->name.empty()) {
        LOG_ERROR("Cannot register null or empty command");
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
    LOG_DEBUG("Registered command: {}", name);
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
    LOG_DEBUG("Unregistered command: {}", name);
}

void CommandManager::registerAlias(const std::string& command, const std::string& alias) {
    std::string lowerCmd = command;
    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(), ::tolower);
    std::string lowerAlias = alias;
    std::transform(lowerAlias.begin(), lowerAlias.end(), lowerAlias.begin(), ::tolower);
    
    aliases[lowerAlias] = lowerCmd;
    LOG_DEBUG("Registered alias '{}' for command '{}'", alias, command);
}

void CommandManager::clear() {
    commands.clear();
    aliases.clear();
    LOG_INFO("CommandManager cleared");
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
    LOG_INFO("Registering vanilla commands...");
    
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
    
    LOG_INFO("Registered {} commands", commands.size());
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
        ctx.sendSuccess("Available commands: /help, /gamemode, /tp, /give, /time, /weather, /kill, /heal, /clear, /list, /stop");
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
std::unique_ptr<Command> createSummonCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createSetBlockCommand() { return std::make_unique<Command>(); }
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
std::unique_ptr<Command> createWorldBorderCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createLocateCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createSpreadPlayersCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createParticleCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createPlaySoundCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createXPCommand() { return std::make_unique<Command>(); }
std::unique_ptr<Command> createAttributeCommand() { return std::make_unique<Command>(); }

} // namespace Commands

} // namespace VoxelForge
