#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "debug.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

static constexpr char NAME[] = "app";
static constexpr int INIT_WINDOW_WIDTH = 800;
static constexpr int INIT_WINDOW_HEIGHT = 600;
static constexpr uint32_t RESET_FLAGS = BGFX_RESET_VSYNC;

bgfx::PlatformData pd;
bgfx::Init init;

SDL_Window* window = nullptr;

uint8_t colorChannel(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    return static_cast<uint8_t>(value * 255.0);
}

uint32_t rgba(double red, double green, double blue, double alpha = 1.0)
{
    return (uint32_t(colorChannel(red)) << 24)
        | (uint32_t(colorChannel(green)) << 16)
        | (uint32_t(colorChannel(blue)) << 8)
        | uint32_t(colorChannel(alpha));
}

uint32_t animatedColor(double time, double phase)
{
    return rgba(
        0.50 + 0.50 * std::sin(time * 0.90 + phase),
        0.50 + 0.50 * std::sin(time * 1.10 + phase + 2.10),
        0.50 + 0.50 * std::sin(time * 1.35 + phase + 4.20));
}

void clearRect(uint16_t viewId, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, color, 1.0f, 0);
    bgfx::setViewRect(viewId, x, y, width, height);
    bgfx::touch(viewId);
}

void render()
{
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    if (width <= 0 || height <= 0)
    {
        SDL_GetWindowSize(window, &width, &height);
    }

    static int lastWidth = 0;
    static int lastHeight = 0;
    if (width != lastWidth || height != lastHeight)
    {
        bgfx::reset(uint32_t(width), uint32_t(height), RESET_FLAGS);
        lastWidth = width;
        lastHeight = height;
    }

    const double time = double(SDL_GetTicks()) / 1000.0;
    const double minDimension = double(std::min(width, height));
    uint16_t viewId = 0;

    clearRect(viewId++, 0, 0, uint16_t(width), uint16_t(height), 0x080A12FF);

    for (int i = 0; i < 12; ++i)
    {
        const double wave = 0.5 + 0.5 * std::sin(time * 1.4 + double(i) * 0.72);
        const int inset = int(minDimension * (0.035 + double(i) * 0.032 + wave * 0.014));
        const int rectWidth = width - inset * 2;
        const int rectHeight = height - inset * 2;
        if (rectWidth <= 0 || rectHeight <= 0)
        {
            break;
        }

        clearRect(
            viewId++,
            uint16_t(inset),
            uint16_t(inset),
            uint16_t(rectWidth),
            uint16_t(rectHeight),
            animatedColor(time, double(i) * 0.48));
    }

    for (int i = 0; i < 18; ++i)
    {
        const double phase = double(i) * 0.77;
        const int size = int(minDimension * (0.035 + 0.018 * (1.0 + std::sin(time * 1.7 + phase))));
        const double centerX = double(width) * (0.50 + 0.36 * std::sin(time * 0.65 + phase));
        const double centerY = double(height) * (0.50 + 0.36 * std::cos(time * 0.82 + phase * 1.31));
        const int x = std::clamp(int(centerX) - size / 2, 0, std::max(0, width - size));
        const int y = std::clamp(int(centerY) - size / 2, 0, std::max(0, height - size));

        clearRect(
            viewId++,
            uint16_t(x),
            uint16_t(y),
            uint16_t(size),
            uint16_t(size),
            animatedColor(time * 1.6, phase + 1.2));
    }

    bgfx::setDebug(BGFX_DEBUG_TEXT);
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(2, 1, 0x4f, "SDL3 + bgfx");
    bgfx::dbgTextPrintf(2, 2, 0x7f, "%dx%d animated view clears", width, height);

    bgfx::frame();
}

bool SDLCALL eventcallback(void *userdata, SDL_Event *event)
{            
    switch (event->type)
    {
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            render();
            break;
    }
    return true;
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    // Create an application window with the following settings:
    constexpr SDL_WindowFlags WFLAGS = SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    window = SDL_CreateWindow("main", // window title
        640,                          // width, in pixels
        480,                          // height, in pixels
        WFLAGS                        // flags - see below
    );

    // Check that the window was successfully created
    if (window == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    // This call should be only used on platforms that don't
    // allow creating separate rendering thread. If it is called before
    // bgfx::init, render thread won't be created by bgfx::init call.
    bgfx::renderFrame();

    // initialize bgfx
    pd.nwh = SDL_GetPointerProperty(SDL_GetWindowProperties(window),
        SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    if (pd.nwh == nullptr)
    {
        DEBUG_PRINT("Failed to get valid window handle\n");
        return 1;
    }
    init.type = bgfx::RendererType::Count; // auto choose renderer
    init.resolution.width = INIT_WINDOW_WIDTH;
    init.resolution.height = INIT_WINDOW_HEIGHT;
    init.resolution.reset = RESET_FLAGS;
    init.platformData = pd;

    if (!bgfx::init(init))
    {
        DEBUG_PRINT("bgfx initialization failed\n");
        return 1;
    }

    if (!SDL_AddEventWatch(eventcallback, nullptr))
    {
        DEBUG_PRINT("Failed to create callback%s\n", SDL_GetError());
    }
    
    // The window is open: could enter program loop here (see SDL_PollEvent()
    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {  // poll until all events are handled!
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            }
        }

        render();
    }
    
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
