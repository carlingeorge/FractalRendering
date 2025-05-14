#pragma once
#include <thread>
#include <mutex>
#include <array>
#include <chrono>

#include "utils/va_grid.hpp"
#include "utils/palette.hpp"
#include "utils/thread_pool.hpp"
#include "utils/to_string.hpp"
#include "utils/number_generator.hpp"
#include "utils/image_loader.hpp"

#include "config.hpp"


template<typename TFloatType>


sf::Color julia_iter(sf::Vector2<TFloatType> pos, ImageLoader* loader)
{
    uint32_t i{0};
    TFloatType zr{pos.x};
    TFloatType zi{pos.y};
    TFloatType mod = zr * zr + zi * zi;

    double multiplier = 1.0f;

    TFloatType dist = 1e20f;
    while (mod < TFloatType{4.0} && i < Config::max_iteration) {
        const TFloatType tmp = zr;

        zr = zr * zr - zi * zi + Config::julia_r;//pos.x;//
        zi = TFloatType{2.0} * zi * tmp + Config::julia_i; // + pos.y;//

        mod = zr * zr + zi * zi;

        dist = std::min(dist,mod);



        if(loader->isIn(zr*multiplier,zi*multiplier)){
            return loader->getValue(zr*multiplier,zi*multiplier);
        }

        
        ++i;
    }
    
    //mod = std::sqrt(dist);
    TFloatType adjusted_dist = std::sqrt(dist) / 2 * 255;
    //return static_cast<float>((adjusted_dist)*Config::max_iteration) - static_cast<float>(log2(std::max(TFloatType(1.0), log2(adjusted_dist))));
    return {adjusted_dist,adjusted_dist,adjusted_dist};
}

template<typename TFloatType>
struct RenderState
{
    VertexArrayGrid         grid;
    TFloatType              zoom     = 1.0;
    sf::Vector2<TFloatType> offset   = {0.0, 0.0};

    RenderState(uint32_t width, uint32_t height)
        : grid(width, height)
    {}
};




template<typename TFloatType>
struct AsyncRenderer
{
    RenderState<TFloatType> states[2];

    TFloatType              requested_zoom = 1.0;
    sf::Vector2<TFloatType> requested_center = {};

    TFloatType              render_zoom   = 1.0;
    sf::Vector2<TFloatType> render_center = {};

    ImageLoader loader = ImageLoader();

    uint32_t state_idx   = 0;
    uint32_t texture_idx = 0;

    std::array<sf::Vector2<TFloatType>, 32> anti_aliasing_offsets;
    sf::RenderTexture                       textures[2];

    tp::ThreadPool thread_pool;

    Palette palette;

    float fade_time = Config::fade_time;

    sf::Color hexToColor(const std::string& hex) const {
        unsigned int rgb;
        std::stringstream ss;
        ss << std::hex << hex.substr(1);  // skip the #
        ss >> rgb;

        return sf::Color{(rgb >> 16) & 0xFF,(rgb >> 8) & 0xFF,rgb & 0xFF};
    }

    void loadPalette() {
        std::array<sf::Color, 18> colors;
        std::ifstream file("../palette/light-theme.json");
        if (!file.is_open()) {
            throw(12);
            return;
        }

        std::string line;
        int index = 0;
        bool not_background = true;
        
        while (std::getline(file, line) && index < colors.size()) {
            size_t colon = line.find(':');
            size_t quote1 = line.find('"', colon);
            size_t quote2 = line.find('"', quote1 + 1);
            if (quote1 != std::string::npos && quote2 != std::string::npos) {
                std::string hex = line.substr(quote1 + 1, quote2 - quote1 - 1);


                    colors[index++] = hexToColor(hex);
                
                
            }
        }

        file.close();

    
        for(int i = 1;i<colors.size();i++){
            palette.addColorPoint((i-1)/(17.0f), colors[i]);

        }

        palette.addColorPoint(1.0f,colors[0]);

    }

    AsyncRenderer(uint32_t width, uint32_t height, TFloatType zoom_)
        : thread_pool{16}
        , states{RenderState<TFloatType>(width, height), RenderState<TFloatType>(width, height)}
    {
        requested_zoom = zoom_;
        //loadPalette();
        loader.loadImage("../palette/blurred.png");
        
        palette.addColorPoint(0.0f,sf::Color{0,0,0});

        palette.addColorPoint(1.0f,sf::Color{255,255,255});
    

        // Precompute Monte Carlo offsets
        for (auto& o : anti_aliasing_offsets) {
            o.x = RNGf::getUnder(1.0f);
            o.y = RNGf::getUnder(1.0f);
        }
        // If only one sample is used, no offset
        anti_aliasing_offsets[0] = {};

        for (uint32_t i{2}; i--;) {
            textures[i].create(width, height);
            textures[i].setSmooth(true);
        }
    }

    ~AsyncRenderer()
    {
        thread_pool.waitForCompletion();
    }

    void generate()
    {
        // Get the back buffer
        RenderState<TFloatType>& state = states[!state_idx];
        VertexArrayGrid&         grid  = state.grid;
        render_zoom   = requested_zoom;
        render_center = requested_center;

        // Generate colors
        const sf::Vector2<TFloatType> win_center = static_cast<TFloatType>(0.5) * sf::Vector2<TFloatType>{static_cast<float>(grid.size.x),
                                                                                                          static_cast<float>(grid.size.y)};
        const uint32_t slice_height = grid.size.y / thread_pool.m_thread_count;
        thread_pool.m_queue.m_remaining_tasks = thread_pool.m_thread_count;
        for (uint32_t k{0}; k < thread_pool.m_thread_count; ++k) {
            thread_pool.addTaskNoIncrement([=] {
                constexpr float    sample_coef  = 1.0f / static_cast<float>(Config::samples_count);
                auto&     grid                  = states[!state_idx].grid;
                for (uint32_t x{0}; x < grid.size.x; ++x) {
                    for (uint32_t y{k * slice_height}; y < (k+1) * slice_height; ++y) {
                        const TFloatType xf = (static_cast<TFloatType>(x) - win_center.x) / render_zoom + render_center.x;
                        const TFloatType yf = (static_cast<TFloatType>(y) - win_center.y) / render_zoom + render_center.y;
                        // AA color accumulator
                        sf::Vector3f color_vec;
                        // Monte Carlo color integration
                        /*
                        for (uint32_t i{Config::samples_count}; i--;) {
                            const sf::Vector2<TFloatType> off        = anti_aliasing_offsets[i] / render_zoom;
                            const sf::Color                   iter       = ;
                            const float                   iter_ratio = iter / static_cast<float>(Config::max_iteration);
                            color_vec += {iter;//palette.getColorVec(iter_ratio);
                        }*/
                        // Update color
                        const sf::Color c = julia_iter<TFloatType>({xf, yf},&loader);
                        grid.setCellColor(x, y, c);
                    }
                }
            });
        }
    }

    void render(TFloatType zoom, sf::Vector2<TFloatType> center, sf::RenderTarget& target)
    {
        // Check if background rendering is ready
        if (fade_time >= Config::fade_time && thread_pool.isDone())
        {
            // Update state
            auto& state  = states[!state_idx];
            state.zoom   = render_zoom;
            state.offset = render_center;
            // Swap buffers
            state_idx = !state_idx;
            // Update texture
            textures[!texture_idx].draw(states[state_idx].grid.va);
            textures[!texture_idx].display();
            texture_idx = !texture_idx;
            fade_time = 0.0f;
            // Start next render
            generate();
        }

        requested_center = center;
        requested_zoom   = zoom;
        const auto scale = getStateScale(state_idx);
        sf::Sprite sprite_new(textures[texture_idx].getTexture());
        const auto bounds = sprite_new.getGlobalBounds();
        const sf::Vector2f origin = sf::Vector2f{bounds.width, bounds.height} * 0.5f;
        sprite_new.setOrigin(origin);
        sprite_new.setPosition(origin + getStateOffset(state_idx));
        sprite_new.setScale(scale, scale);

        const auto scale_old  = getStateScale(!state_idx);
        sf::Sprite sprite_old(textures[!texture_idx].getTexture());
        sprite_old.setOrigin(origin);
        sprite_old.setPosition(origin + getStateOffset(!state_idx));
        sprite_old.setScale(scale_old, scale_old);
        const auto alpha = static_cast<uint8_t>(std::max(0.0f, 1.0f - fade_time / Config::fade_time) * 255.0f);
        sprite_old.setColor(sf::Color{255, 255, 255, alpha});

        target.draw(sprite_new);
        target.draw(sprite_old);

        fade_time += 0.016f;
    }

    float getStateScale(uint32_t idx) const
    {
        return static_cast<float>(requested_zoom / states[idx].zoom);
    }

    sf::Vector2<float> getStateOffset(uint32_t idx) const
    {
        return static_cast<sf::Vector2f>((states[idx].offset - requested_center) * states[idx].zoom) * getStateScale(idx);
    }
};