/**
 * @file BlockColor.hpp
 * @brief Block color computation (shared between Renderer and AsyncChunkWorker)
 */

#pragma once

#include <cstdint>

namespace VoxelForge {
namespace BlockColor {

inline uint32_t pack(uint8_t r, uint8_t g, uint8_t b) {
    return r | (g << 8) | (b << 16) | (0xFF << 24);
}

inline uint32_t applyFaceLight(uint32_t color, int face) {
    float brightness;
    switch (face) {
        case 4: brightness = 1.0f; break;
        case 5: brightness = 0.5f; break;
        case 2: case 3: brightness = 0.8f; break;
        case 0: case 1: brightness = 0.65f; break;
        default: brightness = 1.0f; break;
    }
    uint8_t r = (uint8_t)(color & 0xFF);
    uint8_t g = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b = (uint8_t)((color >> 16) & 0xFF);
    return (uint8_t)(r * brightness) | ((uint8_t)(g * brightness) << 8) |
           ((uint8_t)(b * brightness) << 16) | (0xFF << 24);
}

inline unsigned int posHash(int x, int y, int z) {
    unsigned int h = (unsigned int)(x * 374761393u + y * 668265263u + z * 1274126177u);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

inline int colorNoise(int x, int y, int z, int range) {
    return (int)(posHash(x, y, z) % (unsigned int)(range * 2 + 1)) - range;
}

inline uint32_t compute(uint32_t blockId, int face, int wx, int wy, int wz) {
    int n = colorNoise(wx, wy, wz, 10);
    uint32_t base;
    switch (blockId) {
        case 1: base = pack(128+n, 128+n, 128+n); break;
        case 2: base = face == 4
                        ? pack(70+n, 168+n/2, 60+n)
                        : pack(134+n, 116+n/2, 40+n); break;
        case 3: base = pack(134+n, 86+n/2, 40+n); break;
        case 4: base = pack(108+n, 108+n, 108+n); break;
        case 5: base = pack(178+n, 138+n/2, 78+n); break;
        case 6: base = pack(94+n, 66+n/2, 36+n); break;
        case 7: base = pack(184+n, 170+n/2, 130+n); break;
        case 8: base = pack(70+n, 50+n/2, 30+n); break;
        case 9: base = pack(60+n, 42+n/2, 26+n); break;
        case 10: base = pack(86+n, 58+n/2, 32+n); break;
        case 11: base = pack(42+n/3, 130+n/2, 200); break;
        case 12: base = pack(210+n/2, 70+n/3, 15); break;
        case 13: base = pack(220+n, 206+n/2, 150+n); break;
        case 14: base = pack(8, 8, 8); break;
        case 15: base = pack(156+n, 140+n/2, 126+n); break;
        case 16: base = pack(70+n, 70+n, 70+n); break;
        case 17: base = pack(240+n/3, 200+n/3, 20+n/3); break;
        case 18: base = pack(70+n/3, 210+n/3, 220); break;
        case 19: base = pack(160+n, 120+n/2, 60+n); break;
        case 20: base = pack(140+n, 140+n, 140+n); break;
        case 21: base = pack(140+n, 110+n/2, 60+n); break;
        case 22: base = pack(220+n, 190+n/2, 50+n); break;
        case 23: base = pack(200, 200, 210); break;
        case 24: base = pack(148+n, 130+n/2, 116+n); break;
        case 25: base = pack(160+n, 142+n/2, 128+n); break;
        case 26: base = pack(180+n, 178+n/2, 178+n); break;
        case 27: base = pack(190+n, 186+n/2, 182+n); break;
        case 28: base = pack(138+n, 128+n/2, 120+n); break;
        case 29: base = pack(150+n, 142+n/2, 134+n); break;
        case 30: base = pack(80+n/2, 76+n/2, 76+n/2); break;
        case 31: base = pack(86+n, 82+n/2, 80+n); break;
        case 32: base = pack(100+n, 98+n/2, 94+n); break;
        case 33: case 34: case 35: case 36:
            base = pack(118+n, 114+n/2, 108+n); break;
        case 37: base = pack(156+n, 80+n/2, 50+n); break;
        case 38: base = pack(156+n, 120+n/2, 80+n); break;
        case 39: base = pack(100+n, 80+n/2, 60+n); break;
        case 40: base = pack(60+n, 140+n/2, 50+n); break;
        case 41: base = pack(80+n, 60+n/2, 50+n); break;
        case 42: base = pack(140+n, 30+n/3, 30+n/3); break;
        case 43: base = pack(110+n, 30+n/3, 30+n/3); break;
        case 44: base = pack(40+n/3, 50+n/2, 130); break;
        case 45: base = pack(30+n/3, 40+n/2, 100); break;
        case 46: base = pack(30+n/2, 30+n/2, 30+n/2); break;
        case 47: base = pack(210+n/3, 210+n/3, 210+n/3); break;
        case 48: base = pack(240+n/3, 210+n/3, 40+n/3); break;
        case 49: base = pack(80+n/3, 220+n/3, 230+n/3); break;
        case 50: base = pack(40+n/3, 180+n/3, 50+n/3); break;
        case 51: base = pack(190+n/3, 120+n/3, 60+n/3); break;
        case 52: base = pack(30+n/3, 50+n/2, 140); break;
        case 53: base = pack(160+n, 20+n/3, 20+n/3); break;
        case 54: base = pack(98+n, 70+n/2, 40+n); break;
        case 55: base = pack(200+n/3, 190+n/3, 150+n/3); break;
        case 56: base = pack(120+n, 90+n/2, 50+n); break;
        case 57: base = pack(178+n, 120+n/2, 56+n); break;
        case 58: base = pack(60+n, 40+n/2, 24+n); break;
        case 59: base = pack(110+n, 80+n/2, 45+n); break;
        case 60: base = pack(210+n/3, 170+n/3, 140+n/3); break;
        case 61: base = pack(190+n/3, 160+n/3, 110+n/3); break;
        case 62: base = pack(70+n/2, 30+n/2, 70+n/2); break;
        case 63: base = pack(30+n/2, 120+n/2, 110+n/2); break;
        case 64: case 65: case 66: case 67: case 68: case 69: case 70: case 71: {
            int ln = colorNoise(wx, 0, wz, 15);
            int g = 80 + (int)(blockId - 64) * 8 + ln;
            if (g > 200) g = 200;
            base = pack(30 + ln/3, g, 30 + ln/3);
            break;
        }
        case 72: base = pack(130+n, 120+n/2, 110+n); break;
        case 73: base = pack(120+n, 100+n/2, 70+n); break;
        case 74: base = pack(100+n, 130+n/2, 60+n); break;
        case 75: base = pack(110+n, 90+n/2, 60+n); break;
        case 76: base = pack(80+n, 130+n/2, 70+n); break;
        case 77: base = pack(90+n, 85+n/2, 80+n); break;
        case 78: base = pack(200+n, 160+n/2, 100+n); break;
        case 79: base = pack(210+n, 195+n/2, 150+n); break;
        case 80: base = pack(180+n, 110+n/2, 70+n); break;
        case 81: base = pack(110+n, 50+n/2, 50+n); break;
        case 82: base = pack(110+n, 90+n/2, 70+n); break;
        case 83: base = pack(100+n, 80+n/2, 60+n); break;
        case 84: base = pack(80+n, 75+n/2, 80+n); break;
        case 85: base = pack(50+n, 50+n/2, 50+n); break;
        case 86: base = pack(190+n, 170+n/2, 50+n); break;
        case 87: base = pack(220+n/3, 220+n/3, 210+n/3); break;
        case 88: base = pack(20+n/3, 15+n/3, 25+n/3); break;
        case 89: base = pack(50+n/3, 10+n/3, 60+n/3); break;
        case 128: base = pack(230+n/3, 235+n/3, 240+n/3); break;
        case 129: base = pack(160+n, 200+n/2, 230); break;
        case 130: base = pack(170+n, 200+n/2, 220); break;
        case 131: base = pack(130+n, 170+n/2, 210); break;
        case 132: base = pack(160+n, 155+n/2, 140+n); break;
        case 133: base = pack(180+n, 180+n/2, 160+n); break;
        default: base = pack(130+n, 130+n, 130+n); break;
    }
    return applyFaceLight(base, face);
}

} // namespace BlockColor
} // namespace VoxelForge
