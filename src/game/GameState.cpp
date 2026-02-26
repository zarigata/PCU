/**
 * @file GameState.cpp
 * @brief Game state management implementation
 */

#include <VoxelForge/game/GameState.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

GameStateManager::GameStateManager() {
    LOG_INFO("GameStateManager created");
}

GameStateManager::~GameStateManager() {
    states_.clear();
    LOG_INFO("GameStateManager destroyed");
}

void GameStateManager::pushState(std::unique_ptr<GameState> state) {
    if (!states_.empty()) {
        states_.top()->onPause();
    }
    
    states_.push(std::move(state));
    states_.top()->onEnter();
    
    LOG_DEBUG("Pushed game state: {}", states_.top()->getName());
}

void GameStateManager::popState() {
    if (states_.empty()) return;
    
    states_.top()->onExit();
    states_.pop();
    
    if (!states_.empty()) {
        states_.top()->onResume();
    }
    
    LOG_DEBUG("Popped game state");
}

void GameStateManager::changeState(std::unique_ptr<GameState> state) {
    if (!states_.empty()) {
        states_.top()->onExit();
        states_.pop();
    }
    
    states_.push(std::move(state));
    states_.top()->onEnter();
    
    LOG_DEBUG("Changed to game state: {}", states_.top()->getName());
}

void GameStateManager::update(float deltaTime) {
    if (!states_.empty()) {
        states_.top()->onUpdate(deltaTime);
    }
}

void GameStateManager::render() {
    if (!states_.empty()) {
        states_.top()->onRender();
    }
}

void GameStateManager::handleInput() {
    if (!states_.empty()) {
        states_.top()->onHandleInput();
    }
}

GameState* GameStateManager::getCurrentState() {
    if (!states_.empty()) {
        return states_.top().get();
    }
    return nullptr;
}

bool GameStateManager::isEmpty() const {
    return states_.empty();
}

// ============================================================================
// Specific Game States
// ============================================================================

MainMenuState::MainMenuState() {
    LOG_INFO("MainMenuState created");
}

MainMenuState::~MainMenuState() {
    LOG_INFO("MainMenuState destroyed");
}

void MainMenuState::onEnter() {
    LOG_INFO("Entering main menu");
}

void MainMenuState::onExit() {
    LOG_INFO("Exiting main menu");
}

void MainMenuState::onPause() {
    // Main menu doesn't pause
}

void MainMenuState::onResume() {
    // Main menu doesn't resume
}

void MainMenuState::onUpdate(float deltaTime) {
    // Update menu animations
}

void MainMenuState::onRender() {
    // Render main menu UI
}

void MainMenuState::onHandleInput() {
    // Handle menu navigation
}

// ============================================================================
// Playing State
// ============================================================================

PlayingState::PlayingState(World* world) : world_(world) {
    LOG_INFO("PlayingState created");
}

PlayingState::~PlayingState() {
    LOG_INFO("PlayingState destroyed");
}

void PlayingState::onEnter() {
    LOG_INFO("Entering playing state");
}

void PlayingState::onExit() {
    LOG_INFO("Exiting playing state");
}

void PlayingState::onPause() {
    paused_ = true;
}

void PlayingState::onResume() {
    paused_ = false;
}

void PlayingState::onUpdate(float deltaTime) {
    if (paused_) return;
    
    // Update world
    if (world_) {
        // world_->update(deltaTime);
    }
}

void PlayingState::onRender() {
    // Render world
}

void PlayingState::onHandleInput() {
    // Handle player input
}

void PlayingState::setPaused(bool paused) {
    paused_ = paused;
}

// ============================================================================
// Pause Menu State
// ============================================================================

PauseMenuState::PauseMenuState() {
    LOG_INFO("PauseMenuState created");
}

PauseMenuState::~PauseMenuState() {
    LOG_INFO("PauseMenuState destroyed");
}

void PauseMenuState::onEnter() {
    LOG_INFO("Entering pause menu");
}

void PauseMenuState::onExit() {
    LOG_INFO("Exiting pause menu");
}

void PauseMenuState::onPause() {
    // Pause menu doesn't pause
}

void PauseMenuState::onResume() {
    // Pause menu doesn't resume
}

void PauseMenuState::onUpdate(float deltaTime) {
    // Update pause menu animations
}

void PauseMenuState::onRender() {
    // Render pause menu UI (with game world dimmed in background)
}

void PauseMenuState::onHandleInput() {
    // Handle pause menu navigation
}

// ============================================================================
// Loading State
// ============================================================================

LoadingState::LoadingState(const std::string& targetState)
    : targetState_(targetState), progress_(0.0f), complete_(false) {
    LOG_INFO("LoadingState created for: {}", targetState);
}

LoadingState::~LoadingState() {
    LOG_INFO("LoadingState destroyed");
}

void LoadingState::onEnter() {
    LOG_INFO("Entering loading state");
}

void LoadingState::onExit() {
    LOG_INFO("Exiting loading state");
}

void LoadingState::onPause() {
    // Loading doesn't pause
}

void LoadingState::onResume() {
    // Loading doesn't resume
}

void LoadingState::onUpdate(float deltaTime) {
    if (complete_) return;
    
    // Simulate loading progress
    progress_ += deltaTime * 0.2f;
    
    if (progress_ >= 1.0f) {
        progress_ = 1.0f;
        complete_ = true;
        LOG_INFO("Loading complete");
    }
}

void LoadingState::onRender() {
    // Render loading screen with progress bar
}

void LoadingState::onHandleInput() {
    // Loading screen typically doesn't handle input
}

void LoadingState::setProgress(float progress) {
    progress_ = std::clamp(progress, 0.0f, 1.0f);
}

// ============================================================================
// Inventory Screen State
// ============================================================================

InventoryScreenState::InventoryScreenState() {
    LOG_INFO("InventoryScreenState created");
}

InventoryScreenState::~InventoryScreenState() {
    LOG_INFO("InventoryScreenState destroyed");
}

void InventoryScreenState::onEnter() {
    LOG_INFO("Entering inventory screen");
}

void InventoryScreenState::onExit() {
    LOG_INFO("Exiting inventory screen");
}

void InventoryScreenState::onPause() {
    // Inventory screen doesn't pause
}

void InventoryScreenState::onResume() {
    // Inventory screen doesn't resume
}

void InventoryScreenState::onUpdate(float deltaTime) {
    // Update inventory UI
}

void InventoryScreenState::onRender() {
    // Render inventory UI
}

void InventoryScreenState::onHandleInput() {
    // Handle inventory interactions
}

} // namespace VoxelForge
