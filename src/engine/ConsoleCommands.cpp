#include "ConsoleCommands.h"
#include <sstream>
#include <algorithm>

namespace VoxelForge {

ConsoleCommands::ConsoleCommands()
    : logger_(spdlog::default_logger()->clone("console_commands"))
{
    registerDefaultCommands();
}

bool ConsoleCommands::registerCommand(const std::string& name,
                                       const std::string& description,
                                       const std::string& usage,
                                       CommandCallback callback)
{
    if (name.empty()) {
        logger_->error("Cannot register command with empty name");
        return false;
    }

    if (hasCommand(name)) {
        logger_->warn("Command '{}' already registered, overwriting", name);
    }

    commands_[name] = {name, description, usage, std::move(callback)};
    logger_->debug("Registered command: /{}", name);
    return true;
}

bool ConsoleCommands::unregisterCommand(const std::string& name)
{
    auto it = commands_.find(name);
    if (it == commands_.end()) {
        logger_->warn("Command '{}' not found", name);
        return false;
    }

    commands_.erase(it);
    logger_->debug("Unregistered command: /{}", name);
    return true;
}

std::string ConsoleCommands::execute(const std::string& input)
{
    if (input.empty()) {
        return "Error: Empty command";
    }

    if (input[0] != '/') {
        return "Error: Commands must start with '/'";
    }

    std::string name;
    std::vector<std::string> args;

    if (!parseInput(input, name, args)) {
        return "Error: Failed to parse command";
    }

    if (!hasCommand(name)) {
        return fmt::format("Error: Unknown command '/{}'. Type /help for available commands.", name);
    }

    logger_->info("Executing: /{} {}", name, fmt::join(args, " "));

    try {
        return commands_[name].callback(args);
    } catch (const std::exception& e) {
        logger_->error("Command '/{}' failed: {}", name, e.what());
        return fmt::format("Error: {}", e.what());
    }
}

bool ConsoleCommands::hasCommand(const std::string& name) const
{
    return commands_.find(name) != commands_.end();
}

std::vector<std::string> ConsoleCommands::getCommandList() const
{
    std::vector<std::string> names;
    names.reserve(commands_.size());
    for (const auto& [name, _] : commands_) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

const CommandInfo* ConsoleCommands::getCommandInfo(const std::string& name) const
{
    auto it = commands_.find(name);
    return it != commands_.end() ? &it->second : nullptr;
}

bool ConsoleCommands::parseInput(const std::string& input,
                                  std::string& outName,
                                  std::vector<std::string>& outArgs)
{
    std::istringstream iss(input.substr(1)); // Skip leading '/'
    std::string token;

    // First token is the command name
    if (!(iss >> outName)) {
        return false;
    }

    // Remaining tokens are arguments
    while (iss >> token) {
        // Handle quoted strings
        if (token.front() == '"') {
            std::string quoted = token.substr(1);
            while (iss >> token) {
                if (token.back() == '"') {
                    quoted += " " + token.substr(0, token.size() - 1);
                    break;
                }
                quoted += " " + token;
            }
            outArgs.push_back(quoted);
        } else {
            outArgs.push_back(token);
        }
    }

    return true;
}

void ConsoleCommands::registerDefaultCommands()
{
    // /help - List all commands
    registerCommand("help",
        "Show available commands",
        "/help [command]",
        [this](const std::vector<std::string>& args) -> std::string {
            if (args.empty()) {
                std::string result = "Available commands:\n";
                for (const auto& name : getCommandList()) {
                    const auto* info = getCommandInfo(name);
                    if (info) {
                        result += fmt::format("  /{} - {}\n", name, info->description);
                    }
                }
                return result;
            } else {
                const auto* info = getCommandInfo(args[0]);
                if (info) {
                    return fmt::format("/{} - {}\nUsage: {}",
                                       info->name, info->description, info->usage);
                }
                return fmt::format("Unknown command: {}", args[0]);
            }
        });

    // /stop - Stop the server
    registerCommand("stop",
        "Stop the server gracefully",
        "/stop [reason]",
        [](const std::vector<std::string>& args) -> std::string {
            std::string reason = args.empty() ? "Server stopped" : fmt::format("Server stopped: {}", args[0]);
            // TODO: Integrate with actual server shutdown
            return reason;
        });

    // /say - Broadcast a message
    registerCommand("say",
        "Broadcast a message to all players",
        "/say <message>",
        [](const std::vector<std::string>& args) -> std::string {
            if (args.empty()) {
                return "Usage: /say <message>";
            }
            std::string message = fmt::format("[Server] {}", fmt::join(args, " "));
            // TODO: Integrate with actual broadcast system
            return fmt::format("Broadcast: {}", fmt::join(args, " "));
        });

    // /list - List online players (placeholder)
    registerCommand("list",
        "List online players",
        "/list",
        [](const std::vector<std::string>& /*args*/) -> std::string {
            // TODO: Integrate with player manager
            return "Online players: 0";
        });

    // /version - Show server version
    registerCommand("version",
        "Show server version",
        "/version",
        [](const std::vector<std::string>& /*args*/) -> std::string {
            return "VoxelForge Server v0.1.0-alpha";
        });
}

} // namespace VoxelForge