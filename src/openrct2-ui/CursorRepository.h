/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <SDL.h>
#include <functional>
#include <map>
#include <openrct2/interface/Cursors.h>

struct SDL_Cursor;

namespace OpenRCT2::Ui
{
    class CursorRepository
    {
    private:
        class CursorSetHolder
        {
        private:
            SDL_Cursor* _cursors[CURSOR_COUNT] = { nullptr };
#ifdef __SWITCH__
            SDL_Texture* _cursorTextures[CURSOR_COUNT] = { nullptr };
#endif
        public:
            CursorSetHolder(const std::function<SDL_Cursor*(CURSOR_ID)>& getCursor)
            {
                for (size_t i = 0; i < CURSOR_COUNT; i++)
                {
                    _cursors[i] = getCursor(static_cast<CURSOR_ID>(i));
                }
            }
#ifdef __SWITCH__
            CursorSetHolder(const std::function<SDL_Cursor*(CURSOR_ID)>& getCursor, const std::function<SDL_Texture*(CURSOR_ID)>& getCursorTexture)
            {
                for (size_t i = 0; i < CURSOR_COUNT; i++)
                {
                    _cursors[i] = getCursor(static_cast<CURSOR_ID>(i));
                    _cursorTextures[i] = getCursorTexture(static_cast<CURSOR_ID>(i));
                }
            }
#endif
            ~CursorSetHolder()
            {
                for (size_t i = 0; i < CURSOR_COUNT; i++)
                {
                    SDL_FreeCursor(_cursors[i]);
#ifdef __SWITCH__
                    SDL_DestroyTexture(_cursorTextures[i]);
#endif
                }
            }

            SDL_Cursor* getScaledCursor(CURSOR_ID cursorId)
            {
                return _cursors[cursorId];
            }

#ifdef __SWITCH__
            SDL_Texture* getScaledCursorTexture(CURSOR_ID cursorId)
            {
                return _cursorTextures[cursorId];
            }
#endif
        };

        constexpr static int32_t BASE_CURSOR_WIDTH = 32;
        constexpr static int32_t BASE_CURSOR_HEIGHT = 32;

        CURSOR_ID _currentCursor = CURSOR_UNDEFINED;
#ifdef __SWITCH__
        SDL_Texture* _currentCursorTexture = nullptr;
        int _currentCursorHotSpotX = 0;
        int _currentCursorHotSpotY = 0;
        int _currentCursorWidth = 0;
        int _currentCursorHeight = 0;
#endif
        uint8_t _currentCursorScale = 1;

        std::map<uint8_t, CursorSetHolder> _scaledCursors;

    public:
        ~CursorRepository();
        void LoadCursors();
        CURSOR_ID GetCurrentCursor();
#ifdef __SWITCH__
        SDL_Texture* GetCurrentCursorTexture();
        void GetCurrentCursorHotspotAndSize(int* hot_x, int* hot_y, int* w, int* h);
#endif
        void SetCurrentCursor(CURSOR_ID cursorId);
        void SetCursorScale(uint8_t cursorScale);

    private:
        SDL_Cursor* Create(const CursorData* cursorInfo, uint8_t scale);
#ifdef __SWITCH__
        SDL_Texture* CreateTexture(const CursorData* cursorInfo, uint8_t scale);
#endif
        void GenerateScaledCursorSetHolder(uint8_t scale);
        static const CursorData* GetCursorData(CURSOR_ID cursorId);
    };
} // namespace OpenRCT2::Ui
