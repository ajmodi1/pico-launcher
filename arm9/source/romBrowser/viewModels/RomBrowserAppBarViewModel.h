#pragma once
#include "../IRomBrowserController.h"
#include "../FileType/FileType.h"
#include "RomBrowserViewModel.h"

/// @brief View model for the rom browser app bar
class RomBrowserAppBarViewModel
{
public:
    explicit RomBrowserAppBarViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    void NavigateUp()
    {
        _romBrowserController->NavigateUp();
    }

    void ShowDisplaySettings()
    {
        _romBrowserController->ShowDisplaySettings();
    }

    void ShowRecent()
    {
        _romBrowserController->NavigateToPath("recent:");
    }

    void ShowFavorites()
    {
        _romBrowserController->NavigateToPath("favorites:");
    }

    /// @brief Returns whether the currently highlighted item is a favorited game.
    ///        Cheap enough to call every frame (in-memory check only).
    bool IsSelectedItemFavorite()
    {
        const auto& romBrowserViewModel = _romBrowserController->GetRomBrowserViewModel();
        if (!romBrowserViewModel.IsValid())
        {
            return false;
        }
        int selectedItem = romBrowserViewModel->GetSelectedItem();
        auto& fileInfoManager = romBrowserViewModel->GetFileInfoManager();
        if (selectedItem < 0 || selectedItem >= (int)fileInfoManager.GetItemCount())
        {
            return false;
        }
        const auto& item = fileInfoManager.GetItem(selectedItem);
        if (item.GetFileType()->GetClassification() != FileTypeClassification::Game)
        {
            return false;
        }
        return _romBrowserController->IsFavorite(item);
    }

    constexpr RomBrowserLayout GetRomBrowserLayout() const
    {
        return _romBrowserController->GetRomBrowserDisplaySettings().layout;
    }

private:
    IRomBrowserController* _romBrowserController;
};
