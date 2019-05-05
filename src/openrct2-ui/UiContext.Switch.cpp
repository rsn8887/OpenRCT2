/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifdef __SWITCH__

#    include "UiContext.h"

#    include <SDL.h>
#    include <openrct2/common.h>
#    include <openrct2/core/String.hpp>
#    include <openrct2/platform/platform.h>
#    include <openrct2/ui/UiContext.h>
#    include <sstream>
#    include <stdexcept>

namespace OpenRCT2::Ui
{
    class SwitchContext final : public IPlatformUiContext
    {
    private:
    public:
        SwitchContext()
        {
        }

        void SetWindowIcon(SDL_Window* window) override
        {
        }

        bool IsSteamOverlayAttached() override
        {
            return false;
        }

        void ShowMessageBox(SDL_Window* window, const std::string& message) override
        {
            log_verbose(message.c_str());

            STUB();
        }

        std::string ShowFileDialog(SDL_Window* window, const FileDialogDesc& desc) override
        {
            STUB();

            return std::string();
        }

        std::string ShowDirectoryDialog(SDL_Window* window, const std::string& title) override
        {
            log_info(title.c_str());
            STUB();

            return "/switch/openrct2/rct2/";
        }

        void OpenFolder(const std::string& path) override
        {
        }
    };

    IPlatformUiContext* CreatePlatformUiContext()
    {
        return new SwitchContext();
    }
} // namespace OpenRCT2::Ui

#endif // __SWITCHD__
