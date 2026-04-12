/**
 * @file WorldBorder.hpp
 * @brief World border system for VoxelForge
 *
 * Defines a configurable rectangular boundary for the world.
 * Supports smooth size transitions, damage outside border, and
 * per-axis center positioning.
 *
 * Feature: World borders (FEATURE_MATRIX.md §1.1 — High priority)
 */

#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>

namespace VoxelForge {

/**
 * @brief Configurable world boundary
 *
 * The world border is axis-aligned and centered at (centerX, centerZ).
 * Size is the full width/length of the square boundary (so the border
 * extends from center ± size/2 on each axis).
 *
 * A smooth transition can animate the border from one size to another
 * over a given duration. Entities outside the border receive damage
 * per second.
 */
class WorldBorder {
public:
    /**
     * @brief Border behavior when an entity is outside
     */
    enum class DamageMode : uint8_t {
        None,    ///< No damage
        Damage,  ///< Deal damagePerSecond
        Kill     ///< Instantly kill
    };

    WorldBorder() = default;

    // ---- Center ----

    void setCenter(double x, double z) {
        centerX_ = x;
        centerZ_ = z;
    }

    void setCenterX(double x) { centerX_ = x; }
    void setCenterZ(double z) { centerZ_ = z; }

    double getCenterX() const { return centerX_; }
    double getCenterZ() const { return centerZ_; }

    // ---- Size ----

    /**
     * @brief Set the border size immediately (no animation).
     * @param size Full side length of the square border.
     */
    void setSize(double size);

    /**
     * @brief Animate the border to a new size over @p millis milliseconds.
     */
    void interpolateSize(double targetSize, int64_t millis);

    double getSize() const { return currentSize_; }

    /**
     * @brief Current effective size after accounting for any running transition.
     * Call tick() each world tick to advance the animation.
     */
    double getEffectiveSize() const { return currentSize_; }

    // ---- Bounds query ----

    double getMinX() const { return centerX_ - currentSize_ * 0.5; }
    double getMaxX() const { return centerX_ + currentSize_ * 0.5; }
    double getMinZ() const { return centerZ_ - currentSize_ * 0.5; }
    double getMaxZ() const { return centerZ_ + currentSize_ * 0.5; }

    // ---- Containment checks ----

    bool isInside(double x, double z) const {
        double half = currentSize_ * 0.5;
        return x >= centerX_ - half && x <= centerX_ + half &&
               z >= centerZ_ - half && z <= centerZ_ + half;
    }

    bool isInside(const glm::dvec3& pos) const {
        return isInside(pos.x, pos.z);
    }

    bool isInside(const glm::vec3& pos) const {
        return isInside(static_cast<double>(pos.x), static_cast<double>(pos.z));
    }

    /**
     * @brief Distance from the nearest border edge (negative if outside).
     */
    double distanceToBorder(double x, double z) const;

    // ---- Damage settings ----

    void setDamageMode(DamageMode mode) { damageMode_ = mode; }
    DamageMode getDamageMode() const { return damageMode_; }

    void setDamagePerSecond(float dps) { damagePerSecond_ = dps; }
    float getDamagePerSecond() const { return damagePerSecond_; }

    void setSafeZoneBlocks(int blocks) { safeZoneBlocks_ = blocks; }
    int getSafeZoneBlocks() const { return safeZoneBlocks_; }

    // ---- Warning ----

    void setWarningDistance(int blocks) { warningDistance_ = blocks; }
    int getWarningDistance() const { return warningDistance_; }

    void setWarningTimeSeconds(int seconds) { warningTimeSeconds_ = seconds; }
    int getWarningTimeSeconds() const { return warningTimeSeconds_; }

    /**
     * @brief Whether a position is in the warning zone (inside but close to edge).
     */
    bool isInWarningZone(double x, double z) const;

    // ---- Tick ----

    /**
     * @brief Advance border animation. Call once per world tick.
     * @param dtMs Milliseconds elapsed since last tick (typically 50).
     */
    void tick(int64_t dtMs);

    // ---- Transition info ----

    bool isTransitioning() const { return transitioning_; }
    double getTargetSize() const { return targetSize_; }
    int64_t getRemainingTransitionMs() const;

    // ---- Serialization helpers ----

    struct Snapshot {
        double centerX;
        double centerZ;
        double size;
        double targetSize;
        int64_t transitionRemainingMs;
        float damagePerSecond;
        int safeZoneBlocks;
        int warningDistance;
        int warningTimeSeconds;
    };

    Snapshot takeSnapshot() const;
    void applySnapshot(const Snapshot& snap);

private:
    // Center
    double centerX_ = 0.0;
    double centerZ_ = 0.0;

    // Current (effective) size — updated during transitions
    double currentSize_ = 60000000.0; // ~60M blocks default (effectively infinite)

    // Transition state
    bool transitioning_ = false;
    double startSize_ = 0.0;
    double targetSize_ = 0.0;
    int64_t transitionDurationMs_ = 0;
    int64_t transitionElapsedMs_ = 0;

    // Damage
    DamageMode damageMode_ = DamageMode::Damage;
    float damagePerSecond_ = 5.0f;
    int safeZoneBlocks_ = 5; // blocks of grace inside the border edge

    // Warning
    int warningDistance_ = 0;
    int warningTimeSeconds_ = 0;
};

} // namespace VoxelForge
