#pragma once
#include <stdio.h>
#include "../InternalFileInfo.h"
#include "../BmpFileIcon.h"
#include "fat/FastFileRef.h"

/// @brief Internal file info for gba roms.
class alignas(32) GbaInternalFileInfo : public InternalFileInfo
    {
    public:
        explicit GbaInternalFileInfo(const FastFileRef& fastFileRef);

    constexpr const char* GetGameCode() const override { return _gameCode; }

    std::unique_ptr<FileIcon> CreateGameIcon() const override
{
            // Try a custom per-game icon from /_pico/icons/gba/<gameCode>.bmp
        if (_gameCode[0])
{
            char path[64];
            int length = snprintf(path, sizeof(path), "/_pico/icons/gba/%s.bmp", _gameCode);
            if (length > 0 && length < (int)sizeof(path))
{
                auto icon = BmpFileIcon::TryCreateFromPath(path);
                if (icon)
                                        return icon;
}
}
        return nullptr;
}

private:
    char _gameCode[5];
};
