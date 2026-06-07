#pragma once
#include "core/SharedPtr.h"
#include "services/settings/AppSettings.h"

class SdFolder;
class RomBrowserStateMachine;
class RomBrowserViewModel;
class FileInfo;
class TaskQueueBase;
class ICoverRepository;
class ICheatRepository;

/// @brief The kind of virtual folder the rom browser is currently showing.
enum class VirtualFolderKind
{
    None,
    Recent,
    Favorites
};

class IRomBrowserController
{
public:
    virtual ~IRomBrowserController() = 0;

    virtual void NavigateUp() = 0;
    virtual void NavigateToPath(const TCHAR* name) = 0;
    virtual void LaunchFile(const FileInfo& fileInfo) = 0;
    virtual void ShowGameInfo(const FileInfo& fileInfo) = 0;
    virtual void HideGameInfo() = 0;
    virtual void ShowDisplaySettings() = 0;
    virtual void HideDisplaySettings() = 0;

    virtual void Update() = 0;

    virtual const SdFolder& GetSdFolder() const = 0;

    virtual const RomBrowserStateMachine& GetStateMachine() const = 0;

    virtual const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() = 0;

    virtual TaskQueueBase* GetIoTaskQueue() const = 0;
    virtual TaskQueueBase* GetBgTaskQueue() const = 0;
    virtual const ICoverRepository& GetCoverRepository() const = 0;
    virtual const ICheatRepository& GetCheatRepository() const = 0;

    virtual const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const = 0;

    virtual void SetRomBrowserDisplaySettings(
        const RomBrowserDisplaySettings& romBrowserDisplaySettings) = 0;

    virtual const FileInfo& GetTriggerFileInfo() const = 0;

virtual const char* GetThemeName() const = 0;
virtual void CycleTheme(int direction) = 0;

virtual VirtualFolderKind GetVirtualFolderKind() const = 0;
virtual void ToggleFavorite(const FileInfo& fileInfo) = 0;
virtual bool IsFavorite(const FileInfo& fileInfo) const = 0;

/// @brief Returns a counter that changes whenever the favorites list changes,
///        so cached IsFavorite results can be revalidated cheaply.
virtual u32 GetFavoritesVersion() const = 0;

virtual void ShowSearch() = 0;
virtual void HideSearch() = 0;
virtual void CommitSearch(const char* query) = 0;
virtual const char* GetSearchQuery() const = 0;
};

inline IRomBrowserController::~IRomBrowserController() { }
