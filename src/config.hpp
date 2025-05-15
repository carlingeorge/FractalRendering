#pragma once
#include <cstdint>

struct Config
{
    using FloatType = double;

    static constexpr uint32_t  samples_count = 16;
    static constexpr FloatType julia_r       = static_cast<FloatType>(-0.79);
    static constexpr FloatType julia_i       = static_cast<FloatType>(0.15);
    static constexpr float     fade_time     = 0.5f;
    static constexpr uint32_t  max_iteration = 120;
    static constexpr bool      image_sample  = true;
    static constexpr bool      dot_orbit_trap= false;
    static constexpr bool      mandelbrot    = false;
};
