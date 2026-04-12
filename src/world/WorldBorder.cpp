/**
 * @file WorldBorder.cpp
 * @brief World border implementation
 *
 * Implements smooth border transitions, containment checks,
 * damage calculations, and warning zone detection.
 */

#include <VoxelForge/world/WorldBorder.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>

namespace VoxelForge {

// ============================================================================
// Size management
// ============================================================================

void WorldBorder::setSize(double size) {
    currentSize_ = std::max(1.0, size);
    transitioning_ = false;
    transitionElapsedMs_ = 0;
    transitionDurationMs_ = 0;
}

void WorldBorder::interpolateSize(double targetSize, int64_t millis) {
    if (millis <= 0) {
        setSize(targetSize);
        return;
    }
    startSize_ = currentSize_;
    targetSize_ = std::max(1.0, targetSize);
    transitionDurationMs_ = millis;
    transitionElapsedMs_ = 0;
    transitioning_ = true;
    VF_INFO("World border transitioning from {:.1f} to {:.1f} over {}ms",
            startSize_, targetSize_, millis);
}

// ============================================================================
// Containment & distance
// ============================================================================

double WorldBorder::distanceToBorder(double x, double z) const {
    double half = currentSize_ * 0.5;
    double dxMin = x - (centerX_ - half);
    double dxMax = (centerX_ + half) - x;
    double dzMin = z - (centerZ_ - half);
    double dzMax = (centerZ_ + half) - z;

    // Minimum distance to any edge (negative = outside)
    double dx = std::min(dxMin, dxMax);
    double dz = std::min(dzMin, dzMax);
    return std::min(dx, dz);
}

bool WorldBorder::isInWarningZone(double x, double z) const {
    if (warningDistance_ <= 0) return false;
    double dist = distanceToBorder(x, z);
    return dist >= 0.0 && dist <= static_cast<double>(warningDistance_);
}

// ============================================================================
// Tick — advances the transition animation
// ============================================================================

void WorldBorder::tick(int64_t dtMs) {
    if (!transitioning_) return;

    transitionElapsedMs_ += dtMs;

    if (transitionElapsedMs_ >= transitionDurationMs_) {
        // Transition complete
        currentSize_ = targetSize_;
        transitioning_ = false;
        VF_INFO("World border transition complete. Size = {:.1f}", currentSize_);
        return;
    }

    // Linear interpolation
    double t = static_cast<double>(transitionElapsedMs_) /
               static_cast<double>(transitionDurationMs_);
    currentSize_ = startSize_ + (targetSize_ - startSize_) * t;
}

// ============================================================================
// Transition info
// ============================================================================

int64_t WorldBorder::getRemainingTransitionMs() const {
    if (!transitioning_) return 0;
    return std::max(int64_t{0}, transitionDurationMs_ - transitionElapsedMs_);
}

// ============================================================================
// Snapshot
// ============================================================================

WorldBorder::Snapshot WorldBorder::takeSnapshot() const {
    return Snapshot{
        centerX_,
        centerZ_,
        currentSize_,
        targetSize_,
        getRemainingTransitionMs(),
        damagePerSecond_,
        safeZoneBlocks_,
        warningDistance_,
        warningTimeSeconds_
    };
}

void WorldBorder::applySnapshot(const Snapshot& snap) {
    centerX_ = snap.centerX;
    centerZ_ = snap.centerZ;
    currentSize_ = snap.size;
    targetSize_ = snap.targetSize;
    damagePerSecond_ = snap.damagePerSecond;
    safeZoneBlocks_ = snap.safeZoneBlocks;
    warningDistance_ = snap.warningDistance;
    warningTimeSeconds_ = snap.warningTimeSeconds;

    if (snap.transitionRemainingMs > 0) {
        transitioning_ = true;
        transitionDurationMs_ = snap.transitionRemainingMs;
        transitionElapsedMs_ = 0;
        startSize_ = currentSize_;
    } else {
        transitioning_ = false;
    }
}

} // namespace VoxelForge
