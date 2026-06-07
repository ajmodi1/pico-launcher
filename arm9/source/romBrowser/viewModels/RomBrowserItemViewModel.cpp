#include "common.h"
#include "romBrowser/IRomBrowserController.h"
#include "RomBrowserViewModel.h"
#include "romBrowser/FileType/Nds/NdsFileType.h"
#include "RomBrowserItemViewModel.h"

bool RomBrowserItemViewModel::IsFavorite()
{
    if (_index < 0)
    {
        return false;
    }
    u32 version = _romBrowserController->GetFavoritesVersion();
    if (version != _checkedFavoritesVersion)
    {
        _checkedFavoritesVersion = version;
        const auto& item = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager().GetItem(_index);
        _isFavorite = item.GetFileType()->GetClassification() == FileTypeClassification::Game
            && _romBrowserController->IsFavorite(item);
    }
    return _isFavorite;
}

void RomBrowserItemViewModel::Activate()
{
    if (_index >= 0)
    {
        const auto& item = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager().GetItem(_index);
        if (item.GetFileType()->GetClassification() == FileTypeClassification::Folder)
        {
            _romBrowserController->NavigateToPath(item.GetFileName());
        }
        else
        {
            _romBrowserController->LaunchFile(item);
        }
    }
}

void RomBrowserItemViewModel::ShowGameInfo()
{
    if (_index >= 0)
    {
        const auto& item = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager().GetItem(_index);
        if (item.GetFileType() == &NdsFileType::sInstance)
        {
            _romBrowserController->ShowGameInfo(item);
        }
    }
}
