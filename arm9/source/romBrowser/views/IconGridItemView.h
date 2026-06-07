#pragma once
#include <memory>
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "gui/views/View.h"
#include "core/math/Rgb.h"
#include "../FileType/FileIcon.h"
#include "RomBrowserItemInputHandler.h"

class MaterialColorScheme;

class IconGridItemView : public View
{
public:
    class VramToken
    {
        u32 _vramOffset;
        u32 _heartVramOffset;
    public:
        VramToken()
            : _vramOffset(0), _heartVramOffset(0) { }

        explicit VramToken(u32 offset, u32 heartOffset = 0)
            : _vramOffset(offset), _heartVramOffset(heartOffset) { }

        constexpr u32 GetVramOffset() const { return _vramOffset; }
        constexpr u32 GetHeartVramOffset() const { return _heartVramOffset; }
    };

    void InitVram(const VramContext& vramContext) override;
    void Update() override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    void SetIcon(std::unique_ptr<FileIcon> icon)
    {
        _icon = std::move(icon);
        if (_icon)
        {
            _icon->SetVramAddress(_iconVram, _iconVramOffset);
        }
    }

    void UploadIconGraphics() const
    {
        if (_icon)
        {
            _icon->UploadGraphics();
        }
    }

    virtual void SetGraphics(const VramToken& vramToken)
    {
        _heartVramOffset = vramToken.GetHeartVramOffset();
    }

    RomBrowserItemViewModel& GetViewModel() const
    {
        return *_viewModel;
    }

    /// @brief Uploads the small heart badge tiles shown on favorited games.
    /// @return The obj vram offset of the heart tiles, or 0 if no obj vram.
    static u32 UploadHeartGraphics(const VramContext& vramContext);

protected:
    std::unique_ptr<RomBrowserItemViewModel> _viewModel;
    std::unique_ptr<FileIcon> _icon;
    vu16* _iconVram;
    u32 _iconVramOffset;
    u32 _heartVramOffset = 0;
    RomBrowserItemInputHandler _inputHandler;

    /// @brief Draws the heart badge in the bottom right corner of the cell
    ///        when the bound item is a favorited game.
    void DrawHeartBadge(GraphicsContext& graphicsContext,
        const Rgb<8, 8, 8>& backgroundColor, const Rgb<8, 8, 8>& heartColor) const;

    explicit IconGridItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel)
        : _viewModel(std::move(viewModel)), _inputHandler(this, _viewModel.get()) { }
};
