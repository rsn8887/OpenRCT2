/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "DrawingEngineFactory.hpp"

#include <SDL.h>
#include <cmath>
#include <openrct2/Game.h>
#include <openrct2/common.h>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/IDrawingEngine.h>
#include <openrct2/drawing/LightFX.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/ui/UiContext.h>
#include <vector>

#ifdef __SWITCH__
#include "openrct2-ui/switch_video.h"
#endif

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;
using namespace OpenRCT2::Ui;

class HardwareDisplayDrawingEngine final : public X8DrawingEngine
{
private:
    constexpr static uint32_t DIRTY_VISUAL_TIME = 32;

    std::shared_ptr<IUiContext> const _uiContext;
    SDL_Window* _window = nullptr;
    SDL_Renderer* _sdlRenderer = nullptr;
    SDL_Texture* _screenTexture = nullptr;
    SDL_Texture* _scaledScreenTexture = nullptr;
    SDL_PixelFormat* _screenTextureFormat = nullptr;
    uint32_t _paletteHWMapped[256] = { 0 };
#ifdef __ENABLE_LIGHTFX__
    uint32_t _lightPaletteHWMapped[256] = { 0 };
#endif

    // Steam overlay checking
    uint32_t _pixelBeforeOverlay = 0;
    uint32_t _pixelAfterOverlay = 0;
    bool _overlayActive = false;
    bool _pausedBeforeOverlay = false;
    bool _useVsync = true;

    std::vector<uint32_t> _dirtyVisualsTime;

    bool smoothNN = false;

public:
    explicit HardwareDisplayDrawingEngine(const std::shared_ptr<IUiContext>& uiContext)
        : X8DrawingEngine(uiContext)
        , _uiContext(uiContext)
    {
        _window = (SDL_Window*)_uiContext->GetWindow();
    }

    ~HardwareDisplayDrawingEngine() override
    {
        if (_screenTexture != nullptr)
        {
            SDL_DestroyTexture(_screenTexture);
        }
        if (_scaledScreenTexture != nullptr)
        {
            SDL_DestroyTexture(_scaledScreenTexture);
        }
        SDL_FreeFormat(_screenTextureFormat);
        SDL_DestroyRenderer(_sdlRenderer);
    }

    void Initialise() override
    {
        _sdlRenderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | (_useVsync ? SDL_RENDERER_PRESENTVSYNC : 0));
    }

    void SetVSync(bool vsync) override
    {
        if (_useVsync != vsync)
        {
            _useVsync = vsync;
#ifdef __SWITCH__
            if (vsync)
                SDL_GL_SetSwapInterval(1);
            else
                SDL_GL_SetSwapInterval(0);
#else
            SDL_DestroyRenderer(_sdlRenderer);
            Initialise();
            Resize(_uiContext->GetWidth(), _uiContext->GetHeight());
#endif
        }
    }

    void Resize(uint32_t width, uint32_t height) override
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        if (_screenTexture != nullptr)
        {
            SDL_DestroyTexture(_screenTexture);
        }
        SDL_FreeFormat(_screenTextureFormat);

        SDL_RendererInfo rendererInfo = {};
        int32_t result = SDL_GetRendererInfo(_sdlRenderer, &rendererInfo);
        if (result < 0)
        {
            log_warning("HWDisplayDrawingEngine::Resize error: %s", SDL_GetError());
            return;
        }
        uint32_t pixelFormat = SDL_PIXELFORMAT_UNKNOWN;
        for (uint32_t i = 0; i < rendererInfo.num_texture_formats; i++)
        {
            uint32_t format = rendererInfo.texture_formats[i];
            if (!SDL_ISPIXELFORMAT_FOURCC(format) && !SDL_ISPIXELFORMAT_INDEXED(format)
                && (pixelFormat == SDL_PIXELFORMAT_UNKNOWN || SDL_BYTESPERPIXEL(format) < SDL_BYTESPERPIXEL(pixelFormat)))
            {
                pixelFormat = format;
            }
        }

        int32_t scaleQuality = GetContext()->GetUiContext()->GetScaleQuality();
        if (scaleQuality == SCALE_QUALITY_SMOOTH_NN)
        {
            scaleQuality = SCALE_QUALITY_LINEAR;
            smoothNN = true;
        }
        else
        {
            smoothNN = false;
        }

        if (smoothNN)
        {
            if (_scaledScreenTexture != nullptr)
            {
                SDL_DestroyTexture(_scaledScreenTexture);
            }

            char scaleQualityBuffer[4];
            snprintf(scaleQualityBuffer, sizeof(scaleQualityBuffer), "%u", scaleQuality);
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
            _screenTexture = SDL_CreateTexture(_sdlRenderer, pixelFormat, SDL_TEXTUREACCESS_STREAMING, width, height);
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scaleQualityBuffer);
#ifdef __SWITCH__
            // on Switch, it is not enough to scale by the window_scale parameter, we also have to scale to the native res
            int canvas_height = (int32_t) (gConfigGeneral.window_height / gConfigGeneral.window_scale);

            int switch_screen_width;
            int switch_screen_height;
            switch_get_resolution(&switch_screen_width, &switch_screen_height);

            float y_scale = (float) switch_screen_height / (float) canvas_height;

            uint32_t scale = std::ceil(y_scale);
            _scaledScreenTexture = SDL_CreateTexture(
                    _sdlRenderer, pixelFormat, SDL_TEXTUREACCESS_TARGET, width * scale, height * scale);
#else
            uint32_t scale = std::ceil(gConfigGeneral.window_scale);
            _scaledScreenTexture = SDL_CreateTexture(
                _sdlRenderer, pixelFormat, SDL_TEXTUREACCESS_TARGET, width * scale, height * scale);
#endif
        }
        else
        {
            _screenTexture = SDL_CreateTexture(_sdlRenderer, pixelFormat, SDL_TEXTUREACCESS_STREAMING, width, height);
        }

        uint32_t format;
        SDL_QueryTexture(_screenTexture, &format, nullptr, nullptr, nullptr);
        _screenTextureFormat = SDL_AllocFormat(format);

        ConfigureBits(width, height, width);
    }

    void SetPalette(const rct_palette_entry* palette) override
    {
        if (_screenTextureFormat != nullptr)
        {
            for (int32_t i = 0; i < 256; i++)
            {
                _paletteHWMapped[i] = SDL_MapRGB(_screenTextureFormat, palette[i].red, palette[i].green, palette[i].blue);
            }

#ifdef __ENABLE_LIGHTFX__
            if (gConfigGeneral.enable_light_fx)
            {
                auto lightPalette = lightfx_get_palette();
                for (int32_t i = 0; i < 256; i++)
                {
                    auto src = &lightPalette->entries[i];
                    _lightPaletteHWMapped[i] = SDL_MapRGBA(_screenTextureFormat, src->red, src->green, src->blue, src->alpha);
                }
            }
#endif
        }
    }

    void EndDraw() override
    {
        Display();
        if (gShowDirtyVisuals)
        {
            UpdateDirtyVisuals();
        }
    }

protected:
    void OnDrawDirtyBlock(uint32_t left, uint32_t top, uint32_t columns, uint32_t rows) override
    {
        if (gShowDirtyVisuals)
        {
            uint32_t right = left + columns;
            uint32_t bottom = top + rows;
            for (uint32_t x = left; x < right; x++)
            {
                for (uint32_t y = top; y < bottom; y++)
                {
                    SetDirtyVisualTime(x, y, DIRTY_VISUAL_TIME);
                }
            }
        }
    }

private:
    void Display()
    {
#ifdef __ENABLE_LIGHTFX__
        if (gConfigGeneral.enable_light_fx)
        {
            void* pixels;
            int32_t pitch;
            if (SDL_LockTexture(_screenTexture, nullptr, &pixels, &pitch) == 0)
            {
                lightfx_render_to_texture(pixels, pitch, _bits, _width, _height, _paletteHWMapped, _lightPaletteHWMapped);
                SDL_UnlockTexture(_screenTexture);
            }
        }
        else
#endif
#ifdef __SWITCH__
        // window size changes when switching between docked and handheld modes
        int switch_screen_width, switch_screen_height;
        switch_get_resolution(&switch_screen_width, &switch_screen_height);
#endif
        {
            CopyBitsToTexture(_screenTexture, _bits, (int32_t)_width, (int32_t)_height, _paletteHWMapped);
        }
        if (smoothNN)
        {
            SDL_SetRenderTarget(_sdlRenderer, _scaledScreenTexture);
#ifdef __SWITCH__
            //this destrect here shouldn't be needed, but otherwise the image is way too large
            int w, h;
            SDL_QueryTexture(_scaledScreenTexture, nullptr, nullptr, &w, &h);
            SDL_Rect destrect_scaled = {0, 0, w, h};
            SDL_RenderCopy(_sdlRenderer, _screenTexture, nullptr, &destrect_scaled);
#else
            SDL_RenderCopy(_sdlRenderer, _screenTexture, nullptr, nullptr);
#endif
            SDL_SetRenderTarget(_sdlRenderer, nullptr);
#ifdef __SWITCH__
            //this destrect here shouldn't be needed, but otherwise the image is way too large
            SDL_Rect destrect = {0, 0, (int32_t) switch_screen_width, (int32_t) switch_screen_height};
            SDL_RenderCopy(_sdlRenderer, _scaledScreenTexture, nullptr, &destrect);
#else
            SDL_RenderCopy(_sdlRenderer, _scaledScreenTexture, nullptr, nullptr);
#endif
        }
        else
        {
#ifdef __SWITCH__
            //this destrect here shouldn't be needed, but otherwise the image is way too large
            SDL_Rect destrect = {0, 0, (int32_t) switch_screen_width, (int32_t) switch_screen_height};
            SDL_RenderCopy(_sdlRenderer, _screenTexture, nullptr, &destrect);
#else
            SDL_RenderCopy(_sdlRenderer, _screenTexture, nullptr, nullptr);
#endif
        }

        if (gShowDirtyVisuals)
        {
            RenderDirtyVisuals();
        }

        bool isSteamOverlayActive = GetContext()->GetUiContext()->IsSteamOverlayActive();
        if (isSteamOverlayActive && gConfigGeneral.steam_overlay_pause)
        {
            OverlayPreRenderCheck();
        }

#ifdef __SWITCH__
        if (SDL_ShowCursor(SDL_QUERY))
        {
            SDL_Rect pointer_dst;
            int hot_x;
            int hot_y;
            int w;
            int h;
            GetContext()->GetUiContext()->GetCursorHotspotAndSize(&hot_x, &hot_y, &w, &h);
            context_get_cursor_position(&(pointer_dst.x), &(pointer_dst.y));
            if (pointer_dst.x > hot_x) {
                pointer_dst.x = ((pointer_dst.x - hot_x) * switch_screen_width) / gConfigGeneral.window_width;
            } else {
                pointer_dst.x = 0;
            }
            if (pointer_dst.y > hot_y) {
                pointer_dst.y = ((pointer_dst.y - hot_y) * switch_screen_height) / gConfigGeneral.window_height;
            } else {
                pointer_dst.y = 0;
            }
            pointer_dst.w = (w * switch_screen_width) / (gConfigGeneral.window_width);
            pointer_dst.h = (h * switch_screen_height) / (gConfigGeneral.window_height);
            SDL_Texture* pointer_tex = GetContext()->GetUiContext()->GetCursorTexture();
            SDL_RenderCopy(_sdlRenderer, pointer_tex, nullptr, &pointer_dst);
        }
#endif
        SDL_RenderPresent(_sdlRenderer);

        if (isSteamOverlayActive && gConfigGeneral.steam_overlay_pause)
        {
            OverlayPostRenderCheck();
        }
    }

    void CopyBitsToTexture(SDL_Texture* texture, uint8_t* src, int32_t width, int32_t height, const uint32_t* palette)
    {
        void* pixels;
        int32_t pitch;
        if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) == 0)
        {
            int32_t padding = pitch - (width * 4);
            if (pitch == width * 4)
            {
                uint32_t* dst = (uint32_t*)pixels;
                for (int32_t i = width * height; i > 0; i--)
                {
                    *dst++ = palette[*src++];
                }
            }
            else
            {
                if (pitch == (width * 2) + padding)
                {
                    uint16_t* dst = (uint16_t*)pixels;
                    for (int32_t y = height; y > 0; y--)
                    {
                        for (int32_t x = width; x > 0; x--)
                        {
                            const uint8_t lower = *(uint8_t*)(&palette[*src++]);
                            const uint8_t upper = *(uint8_t*)(&palette[*src++]);
                            *dst++ = (lower << 8) | upper;
                        }
                        dst = (uint16_t*)(((uint8_t*)dst) + padding);
                    }
                }
                else if (pitch == width + padding)
                {
                    uint8_t* dst = (uint8_t*)pixels;
                    for (int32_t y = height; y > 0; y--)
                    {
                        for (int32_t x = width; x > 0; x--)
                        {
                            *dst++ = *(uint8_t*)(&palette[*src++]);
                        }
                        dst += padding;
                    }
                }
            }
            SDL_UnlockTexture(texture);
        }
    }

    uint32_t GetDirtyVisualTime(uint32_t x, uint32_t y)
    {
        uint32_t result = 0;
        uint32_t i = y * _dirtyGrid.BlockColumns + x;
        if (_dirtyVisualsTime.size() > i)
        {
            result = _dirtyVisualsTime[i];
        }
        return result;
    }

    void SetDirtyVisualTime(uint32_t x, uint32_t y, uint32_t value)
    {
        uint32_t i = y * _dirtyGrid.BlockColumns + x;
        if (_dirtyVisualsTime.size() > i)
        {
            _dirtyVisualsTime[i] = value;
        }
    }

    void UpdateDirtyVisuals()
    {
        _dirtyVisualsTime.resize(_dirtyGrid.BlockRows * _dirtyGrid.BlockColumns);
        for (uint32_t y = 0; y < _dirtyGrid.BlockRows; y++)
        {
            for (uint32_t x = 0; x < _dirtyGrid.BlockColumns; x++)
            {
                auto timeLeft = GetDirtyVisualTime(x, y);
                if (timeLeft > 0)
                {
                    SetDirtyVisualTime(x, y, timeLeft - 1);
                }
            }
        }
    }

    void RenderDirtyVisuals()
    {
        float scaleX = gConfigGeneral.window_scale;
        float scaleY = gConfigGeneral.window_scale;

        SDL_SetRenderDrawBlendMode(_sdlRenderer, SDL_BLENDMODE_BLEND);
        for (uint32_t y = 0; y < _dirtyGrid.BlockRows; y++)
        {
            for (uint32_t x = 0; x < _dirtyGrid.BlockColumns; x++)
            {
                auto timeLeft = GetDirtyVisualTime(x, y);
                if (timeLeft > 0)
                {
                    uint8_t alpha = (uint8_t)(timeLeft * 5 / 2);

                    SDL_Rect ddRect;
                    ddRect.x = (int32_t)(x * _dirtyGrid.BlockWidth * scaleX);
                    ddRect.y = (int32_t)(y * _dirtyGrid.BlockHeight * scaleY);
                    ddRect.w = (int32_t)(_dirtyGrid.BlockWidth * scaleX);
                    ddRect.h = (int32_t)(_dirtyGrid.BlockHeight * scaleY);

                    SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, alpha);
                    SDL_RenderFillRect(_sdlRenderer, &ddRect);
                }
            }
        }
    }

    void ReadCentrePixel(uint32_t* pixel)
    {
        SDL_Rect centrePixelRegion = { (int32_t)(_width / 2), (int32_t)(_height / 2), 1, 1 };
        SDL_RenderReadPixels(_sdlRenderer, &centrePixelRegion, SDL_PIXELFORMAT_RGBA8888, pixel, sizeof(uint32_t));
    }

    // Should be called before SDL_RenderPresent to capture frame buffer before Steam overlay is drawn.
    void OverlayPreRenderCheck()
    {
        ReadCentrePixel(&_pixelBeforeOverlay);
    }

    // Should be called after SDL_RenderPresent, when Steam overlay has had the chance to be drawn.
    void OverlayPostRenderCheck()
    {
        ReadCentrePixel(&_pixelAfterOverlay);

        // Detect an active Steam overlay by checking if the centre pixel is changed by the gray fade.
        // Will not be triggered by applications rendering to corners, like FRAPS, MSI Afterburner and Friends popups.
        bool newOverlayActive = _pixelBeforeOverlay != _pixelAfterOverlay;

        // Toggle game pause state consistently with base pause state
        if (!_overlayActive && newOverlayActive)
        {
            _pausedBeforeOverlay = gGamePaused & GAME_PAUSED_NORMAL;
            if (!_pausedBeforeOverlay)
            {
                pause_toggle();
            }
        }
        else if (_overlayActive && !newOverlayActive && !_pausedBeforeOverlay)
        {
            pause_toggle();
        }

        _overlayActive = newOverlayActive;
    }
};

std::unique_ptr<IDrawingEngine> OpenRCT2::Ui::CreateHardwareDisplayDrawingEngine(const std::shared_ptr<IUiContext>& uiContext)
{
    return std::make_unique<HardwareDisplayDrawingEngine>(uiContext);
}
