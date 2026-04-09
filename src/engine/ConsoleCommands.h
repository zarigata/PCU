#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <spdlog/spdlog.h>

namespace VoxelForge {

/**
 * @brief Console command callback type
 * @param args Command arguments
 * @return Command result message
 */
using CommandCallback = std::function<std::string(const std::vector<std::string>& args)>;

/**
 * @brief Console command metadata
 */
struct CommandInfo {
    std::string name;           ///< Command name (e.g., "help")
    std::string description;    ///< Brief description
    std::string usage;          ///< Usage syntax
    CommandCallback callback;   ///< Execution callback
};

/**
 * @brief Console Command System
 *
 * Provides a simple command registration and execution system for server console.
 * Supports command registration, parsing, and execution with arguments.
 */
class ConsoleCommands {
public:
    ConsoleCommands();
    ~ConsoleCommands() = default;

    /**
     * @brief Register a new command
     * @param name Command name (without /)
     * @param description Brief description
     * @param usage Usage syntax
     * @param callback Command execution callback
     * @return true if registered successfully
     */
    bool registerCommand(const std::string& name,
                         const std::string& description,
                         const std::string& usage,
                         CommandCallback callback);

    /**
     * @brief Unregister a command
     * @param name Command name
     * @return true if unregistered successfully
     */
    bool unregisterCommand(const std::string& name);

    /**
     * @brief Execute a command string
     * @param input Raw input string (e.g., "/help")
     * @return Command result message
     */
    std::string execute(const std::string& input);

    /**
     * @brief Check if a command exists
     * @param name Command name
     * @return true if command exists
     */
    bool hasCommand(const std::string& name) const;

    /**
     * @brief Get list of all registered commands
     * @return Vector of command names
     */
    std::vector<std::string> getCommandList() const;

    /**
     * @brief Get command info
     * @param name Command name
     * @return Pointer to CommandInfo or nullptr if not found
     */
    const CommandInfo* getCommandInfo(const std::string& name) const;

private:
    /**
     * @brief Parse input string into command name and arguments
     * @param input Raw input string
     * @param outName Output command name
     * @param outArgs Output arguments
     * @return true if parsing successful
     */
    bool parseInput(const std::string& input,
                     std::string& outName,
                     std::vector<std::string>& outArgs);

    /**
     * @brief Register default commands (help, stop, say)
     */
    void registerDefaultCommands();

    std::unordered_map<std::string, CommandInfo> commands_;
    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace VoxelForge