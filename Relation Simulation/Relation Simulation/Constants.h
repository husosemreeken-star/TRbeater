#ifndef RELSIM_CONSTANTS_H
#define RELSIM_CONSTANTS_H
#include <math.h>
#include <cstdint>
#include <numbers>

constexpr float pi = 3.141592653f;
constexpr float FPS = 60.f;
constexpr uint16_t resolution[2] = {800,600};
constexpr float aspect = static_cast<float>(resolution[0])/static_cast<float>(resolution[1]);
constexpr uint32_t inf = ~0U;

constexpr uint32_t WORLD_LIMIT = 10000;

#endif // RELSIM_CONSTANTS_H
