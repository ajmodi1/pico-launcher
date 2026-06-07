#include "common.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "../views/IconGridItemView.h"
#include "../FileType/FileType.h"
#include "gui/GraphicsContext.h"
#include "backIcon.h"
#include "settingsIcon.h"
#include "heartIcon.h"
#include "recentIcon.h"
#include "listIcon.h"
#include "gui/IVramManager.h"
#include "gui/input/InputProvider.h"
#include "RomBrowserBottomScreenView.h"

RomBrowserBottomScreenView::RomBrowserBottomScreenView(
    RomBrowserBottomScreenViewModel* viewModel,
    const RomBrowserDisplayMode* displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    VBlankTextureLoader* vblankTextureLoader)
    : _viewModel(viewModel)
    , _romBrowserViewFactory(romBrowserViewFactory)
    , _romBrowserDisplayMode(displayMode)
    , _themeFileIconFactory(themeFileIconFactory)
    , _romBrowserAppBarView(RomBrowserAppBarView::CreateShared(_viewModel->GetRomBrowserAppBarViewModel(),
        *displayMode, romBrowserViewFactory))
    , _vblankTextureLoader(vblankTextureLoader)
{
    _romBrowserAppBarView->SetParent(this);
}

void RomBrowserBottomScreenView::InitVram(const VramContext& vramContext)
{
    _romBrowserAppBarView->InitVram(vramContext);
}

void RomBrowserBottomScreenView::Update()
{
    _romBrowserAppBarView->Update();
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->Update();
    }
}

void RomBrowserBottomScreenView::Draw(GraphicsContext& graphicsContext)
{
    _romBrowserAppBarView->Draw(graphicsContext);
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->Draw(graphicsContext);
    }
}

void RomBrowserBottomScreenView::VBlank()
{
    _romBrowserAppBarView->VBlank();
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->VBlank();
    }
}

SharedPtr<View> RomBrowserBottomScreenView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!currentFocus)
    {
        return nullptr;
    }
    if (source == _romBrowserAppBarView.GetPointer())
    {
        if (_romBrowserDisplayMode->IsVertical())
        {
            if (direction == FocusMoveDirection::Right)
            {
                return _romBrowserView->MoveFocus(currentFocus, direction, this);
            }
        }
        else
        {
            if (direction == FocusMoveDirection::Down)
            {
                return _romBrowserView->MoveFocus(currentFocus, direction, this);
            }
        }
        return nullptr;
    }
    else if (source == _romBrowserView.GetPointer())
    {
        if (_romBrowserDisplayMode->IsVertical())
        {
            if (direction == FocusMoveDirection::Left)
            {
                return _romBrowserAppBarView->MoveFocus(currentFocus, direction, this);
            }
        }
        else
        {
            if (direction == FocusMoveDirection::Up)
            {
                return _romBrowserAppBarView->MoveFocus(currentFocus, direction, this);
            }
        }
        return nullptr;
    }
    return nullptr;
}

bool RomBrowserBottomScreenView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->NavigateUp();
        return true;
    }
    if (inputProvider.Triggered(InputKey::Start) && _viewModel->IsRomBrowserVisible())
    {
        // open the search keyboard for the current folder
        _viewModel->ShowSearch();
        return true;
    }
    if (inputProvider.Triggered(InputKey::X) && _viewModel->IsRomBrowserVisible())
    {
        // jump to a random game in the current folder
        const auto& romBrowserViewModel = _viewModel->GetRomBrowserViewModel();
        if (romBrowserViewModel.IsValid() &&
            romBrowserViewModel->GetRomBrowserController()->GetVirtualFolderKind() == VirtualFolderKind::None)
        {
            auto& fileInfoManager = romBrowserViewModel->GetFileInfoManager();
            u32 itemCount = fileInfoManager.GetItemCount();
            u32 gameCount = 0;
            for (u32 i = 0; i < itemCount; i++)
                {
                    if (fileInfoManager.GetItem(i).GetFileType()->GetClassification() == FileTypeClassification::Game)
                        gameCount++;
                }
            if (gameCount > 0)
            {
                u32 pick = gRandomGenerator->NextU32(gameCount);
                for (u32 i = 0; i < itemCount; i++)
                    {
                        const auto& item = fileInfoManager.GetItem(i);
                        if (item.GetFileType()->GetClassification() != FileTypeClassification::Game)
                            continue;
                        if (pick == 0)
                        {
                            TCHAR path[256];
                            f_getcwd(path, sizeof(path) / sizeof(path[0]));
                            int len = strlcat(path, "/", sizeof(path));
                            if (len >= 2 && path[len - 2] == '/')
                            {
                                path[len - 1] = 0;
                            }
                            strlcat(path, item.GetFileName(), sizeof(path));
                            romBrowserViewModel->GetRomBrowserController()->NavigateToPath(path);
                            break;
                        }
                        pick--;
                    }
            }
        }
        return true;
    }
    if (inputProvider.Triggered(InputKey::Select) && _viewModel->IsRomBrowserVisible())
    {
        // toggle favorite status of the highlighted game
        const auto& romBrowserViewModel = _viewModel->GetRomBrowserViewModel();
        if (romBrowserViewModel.IsValid())
        {
            int selectedItem = romBrowserViewModel->GetSelectedItem();
            if (selectedItem >= 0 &&
                selectedItem < (int)romBrowserViewModel->GetFileInfoManager().GetItemCount())
            {
                const auto& item = romBrowserViewModel->GetFileInfoManager().GetItem(selectedItem);
                if (item.GetFileType()->GetClassification() == FileTypeClassification::Game)
                {
                    romBrowserViewModel->GetRomBrowserController()->ToggleFavorite(item);
                }
            }
        }
        return true;
    }
    return View::HandleInput(inputProvider, focusManager);
}

void RomBrowserBottomScreenView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    _romBrowserAppBarView->HandlePenDown(touchPoint, focusManager);
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->HandlePenDown(touchPoint, focusManager);
    }
}

void RomBrowserBottomScreenView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    _romBrowserAppBarView->HandlePenMove(touchPoint, focusManager);
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->HandlePenMove(touchPoint, focusManager);
    }
}

void RomBrowserBottomScreenView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    _romBrowserAppBarView->HandlePenUp(lastTouchPoint, focusManager);
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->HandlePenUp(lastTouchPoint, focusManager);
    }
}

void RomBrowserBottomScreenView::RomBrowserViewModelInvalidated(const VramContext& vramContext)
{
    if (_viewModel->GetRomBrowserViewModel().IsValid())
    {
        _romBrowserView = RomBrowserView::CreateShared(
            _viewModel->GetRomBrowserViewModel(), *_romBrowserDisplayMode,
            _themeFileIconFactory, _romBrowserViewFactory, _vblankTextureLoader);
        _romBrowserView->SetParent(this);
        _romBrowserView->InitVram(vramContext);
    }
    else
    {
        _romBrowserView.Reset();
    }
}
