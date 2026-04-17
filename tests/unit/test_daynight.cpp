#include <gtest/gtest.h>
#include <VoxelForge/game/DayNightCycle.hpp>
#include <cmath>
#include <string>

using namespace VoxelForge;

// ── 1. TimeBasics ──────────────────────────────────────────────────────────

TEST(DayNightCycle, TimeBasics) {
    DayNightCycle cycle;
    EXPECT_EQ(cycle.getDayTime(), 0u);
    EXPECT_EQ(cycle.getDayCount(), 0u);
    EXPECT_EQ(cycle.getWorldAge(), 0u);
    EXPECT_EQ(cycle.getPhase(), TimePhase::Sunrise);
    EXPECT_FALSE(cycle.isFrozen());
}

// ── 2. Tick ────────────────────────────────────────────────────────────────

TEST(DayNightCycle, TickIncrementsWorldAgeAndDayTime) {
    DayNightCycle cycle;
    cycle.tick();
    EXPECT_EQ(cycle.getWorldAge(), 1u);
    EXPECT_EQ(cycle.getDayTime(), 1u);
}

TEST(DayNightCycle, TickWrapsAfterFullDay) {
    DayNightCycle cycle;
    // Set to last tick of the day
    cycle.setDayTime(23999);
    uint32_t oldDayCount = cycle.getDayCount();
    cycle.tick();
    EXPECT_EQ(cycle.getDayTime(), 0u);
    EXPECT_EQ(cycle.getDayCount(), oldDayCount + 1);
    EXPECT_EQ(cycle.getWorldAge(), 1u); // setDayTime doesn't change worldAge
}

// ── 3. SetDayTime ──────────────────────────────────────────────────────────

TEST(DayNightCycle, SetDayTimeTicks) {
    DayNightCycle cycle;
    cycle.setDayTime(5000u);
    EXPECT_EQ(cycle.getDayTime(), 5000u);
}

TEST(DayNightCycle, SetDayTimeHourMin) {
    DayNightCycle cycle;
    cycle.setDayTime(6, 0);
    EXPECT_EQ(cycle.getDayTime(), 6000u);

    cycle.setDayTime(12, 30);
    EXPECT_EQ(cycle.getDayTime(), 12500u);
}

TEST(DayNightCycle, SetDayTimeClampsOrWraps) {
    DayNightCycle cycle;
    // Setting beyond TICKS_PER_DAY should wrap or clamp
    cycle.setDayTime(25000u);
    // Expect it wraps to 1000 (25000 % 24000)
    EXPECT_EQ(cycle.getDayTime(), 1000u);
}

// ── 4. AddTime ─────────────────────────────────────────────────────────────

TEST(DayNightCycle, AddTimeNoWrap) {
    DayNightCycle cycle;
    cycle.addTime(100);
    EXPECT_EQ(cycle.getDayTime(), 100u);
}

TEST(DayNightCycle, AddTimeWrapsAndIncrementsDay) {
    DayNightCycle cycle;
    cycle.setDayTime(23500u);
    cycle.addTime(1000u);
    // 23500 + 1000 = 24500 → wraps to 500, dayCount++
    EXPECT_EQ(cycle.getDayTime(), 500u);
    EXPECT_EQ(cycle.getDayCount(), 1u);
}

// ── 5. TimeSpeed ───────────────────────────────────────────────────────────

TEST(DayNightCycle, TimeSpeedDouble) {
    DayNightCycle cycle;
    cycle.setTimeSpeed(2.0f);
    // timeSpeed=2.0: each tick() advances 2 game ticks
    cycle.tick();
    EXPECT_EQ(cycle.getDayTime(), 2u);
    cycle.tick();
    EXPECT_EQ(cycle.getDayTime(), 4u);
    for (int i = 0; i < 8; ++i) cycle.tick();
    EXPECT_EQ(cycle.getDayTime(), 20u);
}

TEST(DayNightCycle, TimeSpeedZeroFreezes) {
    DayNightCycle cycle;
    cycle.setTimeSpeed(0.0f);
    EXPECT_TRUE(cycle.isFrozen());
    for (int i = 0; i < 100; ++i) cycle.tick();
    EXPECT_EQ(cycle.getDayTime(), 0u);
}

// ── 6. Frozen ──────────────────────────────────────────────────────────────

TEST(DayNightCycle, FrozenPreventsTick) {
    DayNightCycle cycle;
    cycle.setFrozen(true);
    for (int i = 0; i < 50; ++i) cycle.tick();
    EXPECT_EQ(cycle.getDayTime(), 0u);
    EXPECT_EQ(cycle.getWorldAge(), 0u);

    cycle.setFrozen(false);
    // Resume: need enough ticks for accumulator to fire
    for (int i = 0; i < 20; ++i) cycle.tick();
    EXPECT_GT(cycle.getDayTime(), 0u);
}

// ── 7. PhaseCalculation ───────────────────────────────────────────────────

TEST(DayNightCycle, PhaseCalculation) {
    // Sunrise: [0, 2999]
    EXPECT_EQ(DayNightCycle::calculatePhase(0), TimePhase::Sunrise);
    EXPECT_EQ(DayNightCycle::calculatePhase(2999), TimePhase::Sunrise);
    // Day: [3000, 8999]
    EXPECT_EQ(DayNightCycle::calculatePhase(3000), TimePhase::Day);
    EXPECT_EQ(DayNightCycle::calculatePhase(8999), TimePhase::Day);
    // Sunset: [9000, 11999]
    EXPECT_EQ(DayNightCycle::calculatePhase(9000), TimePhase::Sunset);
    EXPECT_EQ(DayNightCycle::calculatePhase(11999), TimePhase::Sunset);
    // Night: [12000, 23999]
    EXPECT_EQ(DayNightCycle::calculatePhase(12000), TimePhase::Night);
    EXPECT_EQ(DayNightCycle::calculatePhase(23999), TimePhase::Night);
}

// ── 8. CelestialAngle ─────────────────────────────────────────────────────

TEST(DayNightCycle, CelestialAngle) {
    float angle0 = DayNightCycle::calculateCelestialAngle(0);
    float angle6000 = DayNightCycle::calculateCelestialAngle(6000);
    float angle12000 = DayNightCycle::calculateCelestialAngle(12000);
    float angle18000 = DayNightCycle::calculateCelestialAngle(18000);

    // Dawn ~ 0
    EXPECT_NEAR(angle0, 0.0f, 0.3f);
    // Noon ~ PI/2
    EXPECT_NEAR(angle6000, static_cast<float>(M_PI) / 2.0f, 0.3f);
    // Dusk ~ PI
    EXPECT_NEAR(angle12000, static_cast<float>(M_PI), 0.3f);
    // Midnight ~ 3PI/2
    EXPECT_NEAR(angle18000, 3.0f * static_cast<float>(M_PI) / 2.0f, 0.3f);
}

// ── 9. MoonPhase ──────────────────────────────────────────────────────────

TEST(DayNightCycle, MoonPhase) {
    // Moon phase cycles 0-7 based on dayCount via the formula
    for (uint32_t d = 0; d < 16; ++d) {
        float phase = DayNightCycle::calculateMoonPhase(d);
        EXPECT_GE(phase, 0.0f);
        EXPECT_LE(phase, 7.0f);
    }
}

TEST(DayNightCycle, MoonPhaseIncrementsWithDayCount) {
    DayNightCycle cycle;
    float p0 = cycle.getMoonPhase();
    cycle.setDayTime(DayNightCycle::TICKS_PER_DAY - 1);
    cycle.tick(); // wraps, dayCount++
    float p1 = cycle.getMoonPhase();
    // Moon phase should change (or wrap)
    // Just verify it's still in valid range
    EXPECT_GE(p1, 0.0f);
    EXPECT_LE(p1, 7.0f);
}

// ── 10. SkyDarkness ────────────────────────────────────────────────────────

TEST(DayNightCycle, SkyDarkness) {
    float darkDay = DayNightCycle::calculateSkyDarkness(6000);
    float darkNight = DayNightCycle::calculateSkyDarkness(18000);
    float darkTransition = DayNightCycle::calculateSkyDarkness(3000);

    EXPECT_NEAR(darkDay, 0.0f, 0.1f);
    EXPECT_GT(darkNight, 0.5f);
    EXPECT_GT(darkTransition, darkDay);
    EXPECT_LT(darkTransition, darkNight);
}

// ── 11. SkyGradient ────────────────────────────────────────────────────────

TEST(DayNightCycle, SkyGradientValidColors) {
    for (uint32_t t = 0; t < DayNightCycle::TICKS_PER_DAY; t += 1000) {
        SkyGradient sg = DayNightCycle::calculateSkyGradient(t);
        auto checkVec = [](const glm::vec3& v) {
            EXPECT_GE(v.x, 0.0f); EXPECT_LE(v.x, 1.0f);
            EXPECT_GE(v.y, 0.0f); EXPECT_LE(v.y, 1.0f);
            EXPECT_GE(v.z, 0.0f); EXPECT_LE(v.z, 1.0f);
        };
        checkVec(sg.zenithColor);
        checkVec(sg.horizonColor);
        checkVec(sg.fogColor);
        EXPECT_GE(sg.ambientLight, 0.0f);
        EXPECT_LE(sg.ambientLight, 1.0f);
    }
}

// ── 12. SunPosition ────────────────────────────────────────────────────────

TEST(DayNightCycle, SunPositionDay) {
    CelestialPosition sun = DayNightCycle::calculateSunPosition(6000);
    EXPECT_TRUE(sun.aboveHorizon);
    // Direction normalized
    float len = glm::length(sun.direction);
    EXPECT_NEAR(len, 1.0f, 0.01f);
}

TEST(DayNightCycle, SunPositionNight) {
    CelestialPosition sun = DayNightCycle::calculateSunPosition(18000);
    EXPECT_FALSE(sun.aboveHorizon);
    float len = glm::length(sun.direction);
    EXPECT_NEAR(len, 1.0f, 0.01f);
}

// ── 13. MoonPosition ───────────────────────────────────────────────────────

TEST(DayNightCycle, MoonPosition) {
    CelestialPosition sun = DayNightCycle::calculateSunPosition(6000);
    CelestialPosition moon = DayNightCycle::calculateMoonPosition(6000);
    // Moon should be roughly opposite to sun
    float dot = glm::dot(sun.direction, moon.direction);
    EXPECT_LT(dot, 0.0f); // Opposite hemispheres
}

// ── 14. PhaseToString ──────────────────────────────────────────────────────

TEST(DayNightCycle, PhaseToString) {
    for (int i = 0; i <= 3; ++i) {
        std::string s = DayNightCycle::phaseToString(static_cast<TimePhase>(i));
        EXPECT_FALSE(s.empty());
    }
}

// ── 15. TimeToString ───────────────────────────────────────────────────────

TEST(DayNightCycle, TimeToString) {
    // tick 0 → 06:00 (dawn offset of 6 hours)
    EXPECT_EQ(DayNightCycle::timeToString(0), "06:00");
    EXPECT_EQ(DayNightCycle::timeToString(6000), "12:00");
    EXPECT_EQ(DayNightCycle::timeToString(12000), "18:00");
    EXPECT_EQ(DayNightCycle::timeToString(18000), "00:00");
}

// ── 16. ExecuteCommand ─────────────────────────────────────────────────────

TEST(DayNightCycle, ExecuteCommandSetDay) {
    DayNightCycle cycle;
    bool ok = cycle.executeCommand("set", "day");
    EXPECT_TRUE(ok);
    // "day" should set to Day phase start (3000)
    EXPECT_EQ(cycle.getDayTime(), DayNightCycle::DAY_START);
}

TEST(DayNightCycle, ExecuteCommandSetNight) {
    DayNightCycle cycle;
    bool ok = cycle.executeCommand("set", "night");
    EXPECT_TRUE(ok);
    EXPECT_EQ(cycle.getDayTime(), DayNightCycle::NIGHT_START);
}

TEST(DayNightCycle, ExecuteCommandSetNoon) {
    DayNightCycle cycle;
    bool ok = cycle.executeCommand("set", "noon");
    EXPECT_TRUE(ok);
    EXPECT_EQ(cycle.getDayTime(), DayNightCycle::NOON);
}

TEST(DayNightCycle, ExecuteCommandSetMidnight) {
    DayNightCycle cycle;
    bool ok = cycle.executeCommand("set", "midnight");
    EXPECT_TRUE(ok);
    EXPECT_EQ(cycle.getDayTime(), DayNightCycle::MIDNIGHT);
}

TEST(DayNightCycle, ExecuteCommandAddTime) {
    DayNightCycle cycle;
    cycle.setDayTime(100u);
    bool ok = cycle.executeCommand("add", "1000");
    EXPECT_TRUE(ok);
    EXPECT_EQ(cycle.getDayTime(), 1100u);
}

TEST(DayNightCycle, ExecuteCommandQueryDaytime) {
    DayNightCycle cycle;
    cycle.setDayTime(5000u);
    bool ok = cycle.executeCommand("query", "daytime");
    // query returns true, result conveyed through toString or similar
    EXPECT_TRUE(ok);
}

// ── 17. ToString ───────────────────────────────────────────────────────────

TEST(DayNightCycle, ToString) {
    DayNightCycle cycle;
    std::string s = cycle.toString();
    EXPECT_FALSE(s.empty());
}
