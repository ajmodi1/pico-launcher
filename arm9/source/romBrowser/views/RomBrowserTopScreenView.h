#pragma once
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "BannerView.h"
#include "gui/views/LabelView.h"
#include "gui/views/Label2DView.h"
#include "../FileType/FileIcon.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"

class RomBrowserViewModel;
class IRomBrowserViewFactory;
class IFontRepository;

class RomBrowserTopScreenView : public ViewContainer
{
    SHARED_ONLY(RomBrowserTopScreenView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void VBlank() override;

    Rectangle GetBounds() const override
{
            return Rectangle(0, 0, 256, 192);
}

private:
    SharedPtr<RomBrowserViewModel> _viewModel;
    const IThemeFileIconFactory* _themeFileIconFactory;
    SharedPtr<BannerView> _fileInfoView;
    std::unique_ptr<FileIcon> _selectedFileIcon;
    SharedPtr<FileCover> _selectedFileCover;
    int _lastSelectedItem = -1;
    bool _iconGraphicsUploaded = false;
    bool _coverGraphicsUploaded = false;
    bool _showCover;
    // When the display mode does not show the cover on the top screen (cover
    // flow), a hero image is shown there instead when one is available.
    bool _showHero = false;
    Point _coverPosition;
SharedPtr<Label2DView> _clockLabel;
u32 _clockFrames = 0;
int _lastClockMinute = -1;
// shows the active search filter (top-left); only created while one is active
SharedPtr<Label2DView> _searchLabel;

    RomBrowserTopScreenView(SharedPtr<RomBrowserViewModel> viewModel,
        const RomBrowserDisplayMode* displayMode,
        const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
const IFontRepository* fontRepository);
};
