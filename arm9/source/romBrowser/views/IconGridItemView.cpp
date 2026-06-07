#include "common.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/input/InputProvider.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/GraphicsContext.h"
#include "gui/palette/GradientPalette.h"
#include "smallHeartIconFilled.h"
#include "IconGridItemView.h"

void IconGridItemView::InitVram(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _iconVramOffset = objVramManager->Alloc(FILE_ICON_VRAM_SIZE);
        _iconVram = objVramManager->GetVramAddress(_iconVramOffset);
    }
}

u32 IconGridItemView::UploadHeartGraphics(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    if (!objVramManager)
    {
        return 0;
    }
    u32 vramOffset = objVramManager->Alloc(smallHeartIconFilledTilesLen);
    dma_ntrCopy32(3, smallHeartIconFilledTiles,
        objVramManager->GetVramAddress(vramOffset), smallHeartIconFilledTilesLen);
    return vramOffset;
}

void IconGridItemView::DrawHeartBadge(GraphicsContext& graphicsContext,
    const Rgb<8, 8, 8>& backgroundColor, const Rgb<8, 8, 8>& heartColor) const
{
    if (!_heartVramOffset || !_viewModel->IsFavorite())
    {
        return;
    }
    u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(backgroundColor, heartColor), _position.y + 26, _position.y + 42);
    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<16, 16>(
            _position.x + 26,
            _position.y + 26, _heartVramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[0]);
}

void IconGridItemView::Update()
{
    _viewModel->DisposeQueueTaskWhenComplete();

    if (_icon)
    {
        _icon->Update();
    }
}

bool IconGridItemView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    return _inputHandler.HandleInput(inputProvider, focusManager)
        || View::HandleInput(inputProvider, focusManager);
}

void IconGridItemView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenDown(touchPoint, focusManager);
}

void IconGridItemView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenMove(touchPoint, focusManager);
}

void IconGridItemView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenUp(lastTouchPoint, focusManager);
}
