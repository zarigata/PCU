#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace VoxelForge {

enum class TimePhase : uint8_t {
    Sunrise = 0,
    Day = 1,
    Sunset = 2,
    Night = 3
};

struct SkyGradient {
    glm::vec3 zenithColor;
    glm::vec3 horizonColor;
    glm::vec3 fogColor;
    float ambientLight;
};

struct CelestialPosition {
    float angle;
    glm::vec3 direction;
    bool aboveHorizon;
};

struct DayNightState {
    uint32_t dayTime;
    uint32_t dayCount;
    uint32_t worldAge;
    TimePhase phase;
    float celestialAngle;
    float moonPhase;
    float skyDarkness;
    float rainDarkness;
    SkyGradient skyColors;
    CelestialPosition sun;
    CelestialPosition moon;
};

class DayNightCycle {
public:
    static constexpr uint32_t TICKS_PER_DAY = 24000;
    static constexpr uint32_t TICKS_PER_HOUR = 1000;
    static constexpr float TICK_DURATION_SEC = 0.05f;
    static constexpr uint32_t SUNRISE_START = 0;
    static constexpr uint32_t DAY_START = 3000;
    static constexpr uint32_t SUNSET_START = 9000;
    static constexpr uint32_t NIGHT_START = 12000;
    static constexpr uint32_t NOON = 6000;
    static constexpr uint32_t MIDNIGHT = 18000;

    explicit DayNightCycle(uint64_t worldSeed = 0);
    ~DayNightCycle() = default;

    void tick();

    void setDayTime(uint32_t ticks);
    void setDayTime(int hour, int minute);
    void addTime(uint32_t ticks);

    uint32_t getDayTime() const { return state.dayTime; }
    uint32_t getDayCount() const { return state.dayCount; }
    uint32_t getWorldAge() const { return state.worldAge; }
    TimePhase getPhase() const { return state.phase; }

    void setTimeSpeed(float multiplier);
    float getTimeSpeed() const { return timeSpeed; }
    void setFrozen(bool frozen);
    bool isFrozen() const { return timeSpeed == 0.0f; }

    const CelestialPosition& getSunPosition() const { return state.sun; }
    const CelestialPosition& getMoonPosition() const { return state.moon; }
    float getCelestialAngle() const { return state.celestialAngle; }
    float getMoonPhase() const { return state.moonPhase; }

    const SkyGradient& getSkyGradient() const { return state.skyColors; }
    float getSkyDarkness() const { return state.skyDarkness; }
    float getAmbientLight() const { return state.skyColors.ambientLight; }

    const DayNightState& getState() const { return state; }

    static TimePhase calculatePhase(uint32_t dayTime);
    static float calculateCelestialAngle(uint32_t dayTime);
    static float calculateMoonPhase(uint32_t dayTime);
    static float calculateSkyDarkness(uint32_t dayTime);
    static SkyGradient calculateSkyGradient(uint32_t dayTime);
    static CelestialPosition calculateSunPosition(uint32_t dayTime);
    static CelestialPosition calculateMoonPosition(uint32_t dayTime);

    static std::string phaseToString(TimePhase phase);
    static std::string timeToString(uint32_t dayTime);
    std::string toString() const;

    bool executeCommand(const std::string& cmd, const std::string& args);

private:
    DayNightState state{};
    float timeSpeed = 1.0f;
    uint64_t seed;
    float tickAccumulator = 0.0f;

    void updateDerivedState();
    static glm::vec3 lerpColor(const glm::vec3& a, const glm::vec3& b, float t);
    static float lerp(float a, float b, float t);
    static float smoothstep(float edge0, float edge1, float x);
};

} // namespace VoxelForge
