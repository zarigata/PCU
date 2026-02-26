/**
 * @file test_random.cpp
 * @brief Random number generation tests
 */

#include <gtest/gtest.h>
#include <VoxelForge/utils/Random.hpp>
#include <cmath>
#include <unordered_set>
#include <algorithm>
#include <vector>

using namespace VoxelForge;

class RandomTest : public ::testing::Test {
protected:
    XorShift128Plus rng{12345};
};

TEST_F(RandomTest, NextFloatInRange) {
    for (int i = 0; i < 1000; i++) {
        float val = rng.nextFloat();
        EXPECT_GE(val, 0.0f);
        EXPECT_LT(val, 1.0f);
    }
}

TEST_F(RandomTest, NextDoubleInRange) {
    for (int i = 0; i < 1000; i++) {
        double val = rng.nextDouble();
        EXPECT_GE(val, 0.0);
        EXPECT_LT(val, 1.0);
    }
}

TEST_F(RandomTest, NextIntRange) {
    for (int i = 0; i < 100; i++) {
        int val = rng.nextInt(0, 10);
        EXPECT_GE(val, 0);
        EXPECT_LE(val, 10);
    }
}

TEST_F(RandomTest, NextIntMaxOnly) {
    for (int i = 0; i < 100; i++) {
        int val = rng.nextInt(100);
        EXPECT_GE(val, 0);
        EXPECT_LT(val, 100);
    }
}

TEST_F(RandomTest, Deterministic) {
    XorShift128Plus rng1{42};
    XorShift128Plus rng2{42};
    
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(rng1.next(), rng2.next());
    }
}

TEST_F(RandomTest, DifferentSeeds) {
    XorShift128Plus rng1{1};
    XorShift128Plus rng2{2};
    
    bool anyDifferent = false;
    for (int i = 0; i < 10; i++) {
        if (rng1.next() != rng2.next()) {
            anyDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(anyDifferent);
}

TEST_F(RandomTest, SeedStateResets) {
    XorShift128Plus rng1{42};
    
    uint64_t first = rng1.next();
    rng1.seedState(42);
    uint64_t second = rng1.next();
    
    EXPECT_EQ(first, second);
}

TEST_F(RandomTest, NextFloatClosed) {
    for (int i = 0; i < 1000; i++) {
        float val = rng.nextFloatClosed();
        EXPECT_GE(val, 0.0f);
        EXPECT_LE(val, 1.0f);
    }
}

TEST_F(RandomTest, NextFloatSigned) {
    for (int i = 0; i < 1000; i++) {
        float val = rng.nextFloatSigned();
        EXPECT_GE(val, -1.0f);
        EXPECT_LE(val, 1.0f);
    }
}

TEST_F(RandomTest, LargeRangeInt) {
    // Test that we can generate numbers in large ranges
    for (int i = 0; i < 100; i++) {
        int val = rng.nextInt(-1000000, 1000000);
        EXPECT_GE(val, -1000000);
        EXPECT_LE(val, 1000000);
    }
}

TEST_F(RandomTest, UniformityTest) {
    // Test that nextFloat is reasonably uniform
    const int buckets = 10;
    const int samples = 10000;
    int counts[buckets] = {0};
    
    for (int i = 0; i < samples; i++) {
        float val = rng.nextFloat();
        int bucket = std::min(static_cast<int>(val * buckets), buckets - 1);
        counts[bucket]++;
    }
    
    // Each bucket should have roughly samples/buckets values
    int expectedPerBucket = samples / buckets;
    
    for (int i = 0; i < buckets; i++) {
        // Allow 50% deviation
        EXPECT_GT(counts[i], expectedPerBucket * 0.5);
        EXPECT_LT(counts[i], expectedPerBucket * 1.5);
    }
}

TEST_F(RandomTest, ThreadLocalRandom) {
    // Test thread-local Random convenience functions
    float f = Random::nextFloat();
    EXPECT_GE(f, 0.0f);
    EXPECT_LT(f, 1.0f);
    
    int i = Random::nextInt(0, 100);
    EXPECT_GE(i, 0);
    EXPECT_LE(i, 100);
}

TEST_F(RandomTest, RandomChance) {
    // Test that chance() works approximately
    int trueCount = 0;
    for (int i = 0; i < 10000; i++) {
        if (Random::chance(0.3f)) {
            trueCount++;
        }
    }
    
    // Should be around 30% (3000)
    EXPECT_GT(trueCount, 2500);
    EXPECT_LT(trueCount, 3500);
}

TEST_F(RandomTest, RandomBool) {
    int trueCount = 0;
    for (int i = 0; i < 10000; i++) {
        if (Random::nextBool()) {
            trueCount++;
        }
    }
    
    // Should be around 50% (5000)
    EXPECT_GT(trueCount, 4500);
    EXPECT_LT(trueCount, 5500);
}

TEST_F(RandomTest, SeededRandomPosition) {
    SeededRandom sr1{12345};
    SeededRandom sr2{12345};
    
    sr1.setSeed(100, 200);
    sr2.setSeed(100, 200);
    
    // Same position should give same values
    EXPECT_EQ(sr1.nextInt(0, 1000), sr2.nextInt(0, 1000));
}

TEST_F(RandomTest, SeededRandomDifferentPositions) {
    SeededRandom sr{12345};
    
    sr.setSeed(100, 200);
    int val1 = sr.nextInt(0, 1000);
    
    sr.setSeed(101, 200);
    int val2 = sr.nextInt(0, 1000);
    
    // Different positions should likely give different values
    // (though not guaranteed)
    EXPECT_NE(val1, val2);
}

TEST_F(RandomTest, Shuffle) {
    std::vector<int> v1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> v2 = v1;
    
    Random::shuffle(v2.begin(), v2.end());
    
    // Should still contain same elements
    std::sort(v2.begin(), v2.end());
    EXPECT_EQ(v1, v2);
}

TEST_F(RandomTest, Choice) {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    int chosen = Random::choice(v);
    
    bool found = false;
    for (int i : v) {
        if (i == chosen) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(RandomTest, Gaussian) {
    // Test Gaussian distribution
    float sum = 0.0f;
    int count = 10000;
    
    for (int i = 0; i < count; i++) {
        sum += Random::nextGaussian(0.0f, 1.0f);
    }
    
    float mean = sum / count;
    
    // Mean should be close to 0
    EXPECT_GT(mean, -0.1f);
    EXPECT_LT(mean, 0.1f);
}
