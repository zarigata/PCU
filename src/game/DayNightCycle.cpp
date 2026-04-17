#include <VoxelForge/game/DayNightCycle.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace VoxelForge {

DayNightCycle::DayNightCycle(uint64_t worldSeed) : seed(worldSeed) {
    state.dayTime = 0;
    state.dayCount = 0;
    state.worldAge = 0;
    state.rainDarkness = 0.0f;
    updateDerivedState();
    VF_INFO("DayNightCycle initialized: seed={}, time={}", seed, timeToString(0));
}

void DayNightCycle::tick() {
    if (timeSpeed <= 0.0f) return;
    state.worldAge++;
    float advanced = timeSpeed;
    tickAccumulator += advanced - static_cast<float>(static_cast<int32_t>(advanced));
    auto wholeTicks = static_cast<uint32_t>(advanced);
    if (tickAccumulator >= 1.0f) {
        wholeTicks += static_cast<uint32_t>(tickAccumulator);
        tickAccumulator -= static_cast<float>(static_cast<uint32_t>(tickAccumulator));
    }
    uint32_t oldTime = state.dayTime;
    state.dayTime = (state.dayTime + wholeTicks) % TICKS_PER_DAY;
    if (state.dayTime < oldTime) {
        state.dayCount++;
    }
    updateDerivedState();
}

void DayNightCycle::setDayTime(uint32_t ticks) {
    ticks = ticks % TICKS_PER_DAY;
    VF_INFO("DayNightCycle::setDayTime({}) => {} (day {})", ticks, timeToString(ticks), state.dayCount);
    state.dayTime = ticks;
    updateDerivedState();
}

void DayNightCycle::setDayTime(int hour, int minute) {
    // Game time: tick 0 = 6:00, 6000 = 12:00, 12000 = 18:00, 18000 = 0:00
    // So game_hour = (ticks/1000 + 6) % 24 => ticks = ((hour - 6 + 24) % 24) * 1000 + minute * (1000/60)
    uint32_t ticks = ((static_cast<uint32_t>((hour - 6 + 24) % 24)) * TICKS_PER_HOUR)
                   + (static_cast<uint32_t>(minute) * TICKS_PER_HOUR / 60);
    setDayTime(ticks);
}

void DayNightCycle::addTime(uint32_t ticks) {
    VF_INFO("DayNightCycle::addTime({})", ticks);
    uint32_t oldTime = state.dayTime;
    state.dayTime = (state.dayTime + ticks) % TICKS_PER_DAY;
    // Calculate how many full days were added
    uint32_t total = oldTime + ticks;
    state.dayCount += total / TICKS_PER_DAY;
    updateDerivedState();
}

void DayNightCycle::setTimeSpeed(float multiplier) {
    timeSpeed = (multiplier < 0.0f) ? 0.0f : multiplier;
    VF_TRACE("Time speed set to {}", timeSpeed);
}

void DayNightCycle::setFrozen(bool frozen) {
    if (frozen) {
        timeSpeed = 0.0f;
    } else if (timeSpeed == 0.0f) {
        timeSpeed = 1.0f;
    }
}

void DayNightCycle::updateDerivedState() {
    state.phase = calculatePhase(state.dayTime);
    state.celestialAngle = calculateCelestialAngle(state.dayTime);
    state.moonPhase = static_cast<float>(state.dayCount % 8);
    state.skyDarkness = calculateSkyDarkness(state.dayTime);
    state.skyColors = calculateSkyGradient(state.dayTime);
    state.sun = calculateSunPosition(state.dayTime);
    state.moon = calculateMoonPosition(state.dayTime);
}

TimePhase DayNightCycle::calculatePhase(uint32_t dayTime) {
    if (dayTime < DAY_START) return TimePhase::Sunrise;
    if (dayTime < SUNSET_START) return TimePhase::Day;
    if (dayTime < NIGHT_START) return TimePhase::Sunset;
    return TimePhase::Night;
}

float DayNightCycle::calculateCelestialAngle(uint32_t dayTime) {
    return (static_cast<float>(dayTime) / static_cast<float>(TICKS_PER_DAY)) * 2.0f * static_cast<float>(M_PI);
}

float DayNightCycle::calculateMoonPhase(uint32_t dayTime) {
    // Stateless version uses dayTime only; day-based cycling handled in updateDerivedState
    (void)dayTime;
    return 0.0f;
}

float DayNightCycle::calculateSkyDarkness(uint32_t dayTime) {
    float t = static_cast<float>(dayTime);
    if (t < DAY_START) {
        // Sunrise: 0-3000, darkness ramps from ~0.5 to 0.0
        return lerp(0.5f, 0.0f, smoothstep(0.0f, 3000.0f, t));
    }
    if (t < SUNSET_START) {
        return 0.0f; // Full day
    }
    if (t < NIGHT_START) {
        // Sunset: 9000-12000, darkness ramps from 0.0 to 1.0
        return lerp(0.0f, 1.0f, smoothstep(9000.0f, 12000.0f, t));
    }
    if (t < MIDNIGHT) {
        // Night approaching midnight: 12000-18000
        return lerp(1.0f, 1.0f, smoothstep(12000.0f, 18000.0f, t));
    }
    // After midnight: 18000-24000 (wrapping to sunrise)
    // Ramps from 1.0 down, hitting ~0.5 at tick 0
    // Map 18000-24000 to 0-1
    float afterMid = (t - MIDNIGHT) / (TICKS_PER_DAY - MIDNIGHT);
    return lerp(1.0f, 0.5f, smoothstep(0.0f, 1.0f, afterMid));
}

SkyGradient DayNightCycle::calculateSkyGradient(uint32_t dayTime) {
    // Key gradients for each phase
    static constexpr SkyGradient kDay    = {{0.51f, 0.71f, 1.0f}, {0.82f, 0.91f, 1.0f}, {0.75f, 0.85f, 1.0f}, 1.0f};
    static constexpr SkyGradient kSunset = {{0.3f,  0.2f,  0.6f}, {1.0f,  0.4f,  0.1f}, {0.9f,  0.5f,  0.2f}, 0.5f};
    static constexpr SkyGradient kNight  = {{0.01f, 0.01f, 0.04f},{0.02f, 0.02f, 0.08f},{0.02f, 0.02f, 0.06f},0.1f};
    static constexpr SkyGradient kSunrise= {{0.5f,  0.3f,  0.7f}, {1.0f,  0.6f,  0.2f}, {0.9f,  0.6f,  0.3f}, 0.5f};

    float t = static_cast<float>(dayTime);
    SkyGradient result;

    if (dayTime < DAY_START) {
        // 0-3000: Sunrise. Interpolate from night-ish (start of sunrise) to sunrise colors.
        // At tick 0: blend between night and sunrise; at 3000: blend to day
        float p = smoothstep(0.0f, 3000.0f, t);
        result.zenithColor  = lerpColor(kNight.zenithColor, kSunrise.zenithColor, p);
        result.horizonColor = lerpColor(kNight.horizonColor, kSunrise.horizonColor, p);
        result.fogColor     = lerpColor(kNight.fogColor, kSunrise.fogColor, p);
        result.ambientLight = lerp(kNight.ambientLight, kSunrise.ambientLight, p);
    } else if (dayTime < SUNSET_START) {
        // 3000-9000: Day
        result = kDay;
    } else if (dayTime < NIGHT_START) {
        // 9000-12000: Sunset
        float p = smoothstep(9000.0f, 12000.0f, t);
        result.zenithColor  = lerpColor(kDay.zenithColor, kSunset.zenithColor, p);
        result.horizonColor = lerpColor(kDay.horizonColor, kSunset.horizonColor, p);
        result.fogColor     = lerpColor(kDay.fogColor, kSunset.fogColor, p);
        result.ambientLight = lerp(kDay.ambientLight, kSunset.ambientLight, p);
    } else if (dayTime < MIDNIGHT) {
        // 12000-18000: Transition from sunset to full night
        float p = smoothstep(12000.0f, 18000.0f, t);
        result.zenithColor  = lerpColor(kSunset.zenithColor, kNight.zenithColor, p);
        result.horizonColor = lerpColor(kSunset.horizonColor, kNight.horizonColor, p);
        result.fogColor     = lerpColor(kSunset.fogColor, kNight.fogColor, p);
        result.ambientLight = lerp(kSunset.ambientLight, kNight.ambientLight, p);
    } else {
        // 18000-24000: Full night, transitioning toward pre-dawn at the end
        float p = smoothstep(18000.0f, 24000.0f, t);
        result.zenithColor  = lerpColor(kNight.zenithColor, kNight.zenithColor, p);
        result.horizonColor = lerpColor(kNight.horizonColor, kNight.horizonColor, p);
        result.fogColor     = lerpColor(kNight.fogColor, kNight.fogColor, p);
        result.ambientLight = kNight.ambientLight;
    }

    return result;
}

CelestialPosition DayNightCycle::calculateSunPosition(uint32_t dayTime) {
    CelestialPosition pos;
    pos.angle = calculateCelestialAngle(dayTime);
    pos.direction = glm::vec3(std::cos(pos.angle), std::sin(pos.angle), 0.0f);
    pos.aboveHorizon = pos.angle < static_cast<float>(M_PI);
    return pos;
}

CelestialPosition DayNightCycle::calculateMoonPosition(uint32_t dayTime) {
    CelestialPosition pos;
    pos.angle = calculateCelestialAngle(dayTime) + static_cast<float>(M_PI);
    if (pos.angle >= 2.0f * static_cast<float>(M_PI)) {
        pos.angle -= 2.0f * static_cast<float>(M_PI);
    }
    pos.direction = glm::vec3(std::cos(pos.angle), std::sin(pos.angle), 0.0f);
    pos.aboveHorizon = pos.angle < static_cast<float>(M_PI);
    return pos;
}

std::string DayNightCycle::phaseToString(TimePhase phase) {
    switch (phase) {
        case TimePhase::Sunrise: return "Sunrise";
        case TimePhase::Day:     return "Day";
        case TimePhase::Sunset:  return "Sunset";
        case TimePhase::Night:   return "Night";
    }
    return "Unknown";
}

std::string DayNightCycle::timeToString(uint32_t dayTime) {
    // Tick 0 = 6:00, 6000 = 12:00, 12000 = 18:00, 18000 = 0:00
    int totalMinutes = static_cast<int>((static_cast<int64_t>(dayTime) * 60) / TICKS_PER_HOUR);
    totalMinutes += 6 * 60; // offset by 6 hours
    totalMinutes %= 24 * 60;
    int hour = totalMinutes / 60;
    int minute = totalMinutes % 60;
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << hour << ":"
       << std::setfill('0') << std::setw(2) << minute;
    return ss.str();
}

std::string DayNightCycle::toString() const {
    std::ostringstream ss;
    ss << "Time=" << timeToString(state.dayTime)
       << " Phase=" << phaseToString(state.phase)
       << " Day=" << state.dayCount
       << " WorldAge=" << state.worldAge
       << " Darkness=" << std::fixed << std::setprecision(2) << state.skyDarkness
       << " Ambient=" << state.skyColors.ambientLight;
    return ss.str();
}

bool DayNightCycle::executeCommand(const std::string& cmd, const std::string& args) {
    if (cmd == "set") {
        if (args == "day") {
            setDayTime(DAY_START);
        } else if (args == "night") {
            setDayTime(NIGHT_START);
        } else if (args == "noon") {
            setDayTime(NOON);
        } else if (args == "midnight") {
            setDayTime(MIDNIGHT);
        } else {
            // Try parsing as integer ticks
            try {
                uint32_t ticks = static_cast<uint32_t>(std::stoul(args));
                setDayTime(ticks);
            } catch (...) {
                VF_INFO("DayNightCycle: unknown set argument '{}'", args);
                return false;
            }
        }
        return true;
    }
    if (cmd == "add") {
        try {
            uint32_t ticks = static_cast<uint32_t>(std::stoul(args));
            addTime(ticks);
        } catch (...) {
            VF_INFO("DayNightCycle: invalid add argument '{}'", args);
            return false;
        }
        return true;
    }
    if (cmd == "query") {
        // Queries are informational; log the result
        if (args == "daytime") {
            VF_INFO("Daytime: {} ticks ({})", state.dayTime, timeToString(state.dayTime));
        } else if (args == "gametime") {
            VF_INFO("Gametime: {} ticks (day {})", state.worldAge, state.dayCount);
        } else if (args == "day") {
            VF_INFO("Day: {}", state.dayCount);
        } else {
            VF_INFO("DayNightCycle: unknown query '{}'", args);
            return false;
        }
        return true;
    }
    VF_INFO("DayNightCycle: unknown command '{}'", cmd);
    return false;
}

glm::vec3 DayNightCycle::lerpColor(const glm::vec3& a, const glm::vec3& b, float t) {
    return glm::vec3(
        lerp(a.r, b.r, t),
        lerp(a.g, b.g, t),
        lerp(a.b, b.b, t)
    );
}

float DayNightCycle::lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float DayNightCycle::smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace VoxelForge
