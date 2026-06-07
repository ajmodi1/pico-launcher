#pragma once
#include <memory>
#include "fat/ff.h"
#include "FileIcon.h"

/// @brief A 32x32 16-color file icon loaded from a BMP file on the SD card.
///        The BMP must be 32x32, 8 bits per pixel, using only palette entries 0-15.
///        Palette entry 0 is treated as transparent.
class alignas(32) BmpFileIcon : public FileIcon
{
public:
    /// @brief Tries to create an icon from a BMP file at the given path.
    /// @param path The path of the BMP file.
    /// @return The constructed icon, or nullptr when the file does not exist or is not valid.
    static std::unique_ptr<BmpFileIcon> TryCreateFromPath(const TCHAR* path);

    void UploadGraphics() override;
    void Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor) override;

private:
    BmpFileIcon() = default;
    bool Load(const TCHAR* path);

    u8 _tileData[512] alignas(32);
    u16 _palette[16] alignas(32);
};
