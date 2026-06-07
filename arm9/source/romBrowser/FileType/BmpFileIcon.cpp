#include "common.h"
#include <nds/arm9/cache.h>
#include <libtwl/dma/dmaNitro.h>
#include "fat/File.h"
#include "core/math/ColorConverter.h"
#include "gui/PaletteManager.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/GraphicsContext.h"
#include "gui/palette/DirectPalette.h"
#include "BmpFileIcon.h"

std::unique_ptr<BmpFileIcon> BmpFileIcon::TryCreateFromPath(const TCHAR* path)
{
    auto icon = std::unique_ptr<BmpFileIcon>(new BmpFileIcon());
    if (!icon->Load(path))
        return nullptr;
    return icon;
}

bool BmpFileIcon::Load(const TCHAR* path)
{
    File file;
    if (file.Open(path, FA_READ) != FR_OK)
        return false;

    u8 header[0x36 + 16 * 4];
    if (!file.ReadExact(header, sizeof(header)))
    {
        file.Close();
        return false;
    }

    if (header[0] != 'B' || header[1] != 'M')
    {
        file.Close();
        return false;
    }

    u32 dataOffset = header[0xA] | (header[0xB] << 8) | (header[0xC] << 16) | (header[0xD] << 24);
    s32 width = (s32)(header[0x12] | (header[0x13] << 8) | (header[0x14] << 16) | (header[0x15] << 24));
    s32 height = (s32)(header[0x16] | (header[0x17] << 8) | (header[0x18] << 16) | (header[0x19] << 24));
    u16 bitsPerPixel = header[0x1C] | (header[0x1D] << 8);

    bool topDown = height < 0;
    if (topDown)
        height = -height;

    if (width != 32 || height != 32 || bitsPerPixel != 8)
    {
        file.Close();
        return false;
    }

    const u8* paletteData = &header[0x36];
    for (u32 i = 0; i < 16; i++)
    {
        u32 b = paletteData[i * 4 + 0];
        u32 g = paletteData[i * 4 + 1];
        u32 r = paletteData[i * 4 + 2];
        _palette[i] = ColorConverter::ToXBGR555(Rgb<5, 5, 5>(Rgb<8, 8, 8>(r, g, b)));
    }

    u8 pixels[32 * 32];
    if (file.Seek(dataOffset) != FR_OK || !file.ReadExact(pixels, sizeof(pixels)))
    {
        file.Close();
        return false;
    }
    file.Close();

    // Convert the 8 bpp bitmap rows to 4 bpp 8x8 obj tiles.
    for (int y = 0; y < 32; y++)
    {
        int srcY = topDown ? y : 31 - y;
        const u8* row = &pixels[srcY * 32];
        for (int x = 0; x < 32; x += 2)
        {
            u8 lo = row[x] & 0xF;
            u8 hi = row[x + 1] & 0xF;
            int tile = (y >> 3) * 4 + (x >> 3);
            int offset = tile * 32 + (y & 7) * 4 + ((x & 7) >> 1);
            _tileData[offset] = lo | (hi << 4);
        }
    }

    DC_FlushRange(_tileData, sizeof(_tileData));
    DC_FlushRange(_palette, sizeof(_palette));
    return true;
}

void BmpFileIcon::UploadGraphics()
{
    if (_vramAddress != nullptr)
    {
        dma_ntrCopy32(3, _tileData, _vramAddress, 512);
    }
}

void BmpFileIcon::Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor)
{
    if (!graphicsContext.IsVisible(Rectangle(_position, 32, 32)))
        return;

    u32 paletteRowIdx = graphicsContext.GetPaletteManager().AllocRow(
        DirectPalette(_palette), _position.y, _position.y + 32);

    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<32, 32>(_position, _vramOffset >> 7)
        .WithPalette16(paletteRowIdx)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[0]);
}
