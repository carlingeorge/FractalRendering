#pragma once
#include <cstdint>

struct Config
{
    using FloatType = double;

    static constexpr uint32_t  samples_count = 16;
    static constexpr FloatType julia_r       = static_cast<FloatType>(-0.8);
    static constexpr FloatType julia_i       = static_cast<FloatType>(0.17);
    static constexpr float     fade_time     = 1.2f;
    static constexpr uint32_t  max_iteration = 120;
};
