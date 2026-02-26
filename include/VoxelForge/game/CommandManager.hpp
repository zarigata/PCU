/**
 * @file CommandManager.hpp
 * @brief Command system for console and chat commands
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>
#include <glm/glm.hpp>

namespace VoxelForge {

// Forward declarations
class Server;
class Player;
class World;
class EntityManager;

// Command source
enum class CommandSource {
    Player,
    Console,
    CommandBlock,
    Function,
    Script
};

// Command context
struct CommandContext {
    CommandSource source = CommandSource::Console;
    uint32_t executorId = 0;        // Player/entity ID (0 for console)
    std::string executorName;
    World* world = nullptr;
    EntityManager* entityManager = nullptr;
    Server* server = nullptr;
    glm::vec3 position;
    glm::vec3 rotation;
    int permissionLevel = 0;
    
    // Response
    void sendSuccess(const std::string& message);
    void sendError(const std::string& message);
    void sendFeedback(const std::string& message);
};

// Command argument types
enum class ArgumentType {
    String,
    Integer,
    Float,
    Boolean,
    Player,
    Entity,
    BlockPos,
    Vec3,
    Item,
    Block,
    Range,
    Selector
};

// Command argument
struct CommandArgument {
    std::string name;
    ArgumentType type;
    bool optional = false;
    std::vector<std::string> suggestions;  // Auto-complete options
    std::string defaultValue;
};

// Command result
struct CommandResult {
    bool success = false;
    std::string message;
    int affectedCount = 0;
};

// Command definition
class Command {
public:
    std::string name;
    std::string description;
    std::string usage;
    std::vector<std::string> aliases;
    std::vector<CommandArgument> arguments;
    int requiredPermission = 0;
    bool operatorOnly = false;
    bool hidden = false;
    
    using ExecuteFunc = std::function<CommandResult(CommandContext&, const std::vector<std::string>&)>;
    using TabCompleteFunc = std::vector<std::string>(*)(const std::string& partial);
    
    ExecuteFunc execute;
    TabCompleteFunc tabComplete = nullptr;
    
    // Parse argument
    std::optional<int> parseInteger(const std::string& arg, int min = INT_MIN, int max = INT_MAX) const;
    std::optional<float> parseFloat(const std::string& arg, float min = -FLT_MAX, float max = FLT_MAX) const;
    std::optional<bool> parseBool(const std::string& arg) const;
    std::optional<glm::ivec3> parseBlockPos(const std::string& arg) const;
    std::optional<glm::vec3> parseVec3(const std::string& arg) const;
    std::vector<uint32_t> parseSelector(const std::string& arg, CommandContext& ctx) const;
};

// Command manager
class CommandManager {
public:
    CommandManager();
    ~CommandManager();
    
    // Registration
    void registerCommand(std::unique_ptr<Command> command);
    void unregisterCommand(const std::string& name);
    void registerAlias(const std::string& command, const std::string& alias);
    void clear();
    
    // Execution
    CommandResult execute(const std::string& input, CommandContext& context);
    CommandResult executeAs(const std::string& input, uint32_t playerId);
    CommandResult executeConsole(const std::string& input);
    
    // Tab completion
    std::vector<std::string> tabComplete(const std::string& input, CommandContext& context);
    
    // Query
    const Command* getCommand(const std::string& name) const;
    bool hasCommand(const std::string& name) const;
    std::vector<const Command*> getAllCommands() const;
    std::vector<const Command*> getCommandsForPermission(int level) const;
    
    // Vanilla commands
    void registerVanillaCommands();
    
    // Parsing helpers
    std::vector<std::string> tokenize(const std::string& input) const;
    std::string joinArguments(const std::vector<std::string>& args, size_t start = 0) const;
    
private:
    Command* findCommand(const std::string& name);
    const Command* findCommand(const std::string& name) const;
    
    std::unordered_map<std::string, std::unique_ptr<Command>> commands;
    std::unordered_map<std::string, std::string> aliases;  // alias -> command name
};

// Built-in command builders
namespace Commands {
    std::unique_ptr<Command> createGamemodeCommand();
    std::unique_ptr<Command> createTeleportCommand();
    std::unique_ptr<Command> createGiveCommand();
    std::unique_ptr<Command> createTimeCommand();
    std::unique_ptr<Command> createWeatherCommand();
    std::unique_ptr<Command> createKillCommand();
    std::unique_ptr<Command> createHealCommand();
    std::unique_ptr<Command> createClearCommand();
    std::unique_ptr<Command> createEffectCommand();
    std::unique_ptr<Command> createEnchantCommand();
    std::unique_ptr<Command> createSummonCommand();
    std::unique_ptr<Command> createSetBlockCommand();
    std::unique_ptr<Command> createFillCommand();
    std::unique_ptr<Command> createCloneCommand();
    std::unique_ptr<Command> createDifficultyCommand();
    std::unique_ptr<Command> createSeedCommand();
    std::unique_ptr<Command> createHelpCommand();
    std::unique_ptr<Command> createListCommand();
    std::unique_ptr<Command> createStopCommand();
    std::unique_ptr<Command> createOpCommand();
    std::unique_ptr<Command> createDeopCommand();
    std::unique_ptr<Command> createKickCommand();
    std::unique_ptr<Command> createBanCommand();
    std::unique_ptr<Command> createWhitelistCommand();
    std::unique_ptr<Command> createSayCommand();
    std::unique_ptr<Command> createTellCommand();
    std::unique_ptr<Command> createTitleCommand();
    std::unique_ptr<Command> createScoreboardCommand();
    std::unique_ptr<Command> createWorldBorderCommand();
    std::unique_ptr<Command> createLocateCommand();
    std::unique_ptr<Command> createSpreadPlayersCommand();
    std::unique_ptr<Command> createParticleCommand();
    std::unique_ptr<Command> createPlaySoundCommand();
    std::unique_ptr<Command> createXPCommand();
    std::unique_ptr<Command> createAttributeCommand();
}

} // namespace VoxelForge
