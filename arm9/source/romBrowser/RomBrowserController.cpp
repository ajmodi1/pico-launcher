#include "common.h"
#include <array>
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "SdFolderFactory.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/UsrCheatRepositoryFactory.h"
#include "cheats/EmptyCheatRepository.h"
#include "cheats/PicoLoaderCheatDataFactory.h"
#include "RomBrowserController.h"

RomBrowserController::RomBrowserController(
    IAppSettingsService* appSettingsService, TaskQueueBase* ioTaskQueue,
    TaskQueueBase* bgTaskQueue)
    : _appSettingsService(appSettingsService)
    , _ioTaskQueue(ioTaskQueue), _bgTaskQueue(bgTaskQueue)
    , _fileTypeProvider(appSettingsService->GetAppSettings()) { }

void RomBrowserController::NavigateUp()
{
    if (_virtualFolderKind != VirtualFolderKind::None)
    {
        // leave the virtual folder and return to the real folder we came from
        // (_currentRealPath is cached so the UI thread never touches the sd card)
        NavigateToPath(_currentRealPath);
    }
    else
    {
        NavigateToPath("..");
    }
}

void RomBrowserController::NavigateToPath(const TCHAR* name)
{
    StringUtil::Copy(_navigatePath, name, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Navigate);
}

void RomBrowserController::LaunchFile(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

void RomBrowserController::ShowGameInfo(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::ShowGameInfo);
}

void RomBrowserController::HideGameInfo()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideGameInfo);
}

void RomBrowserController::ShowDisplaySettings()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowDisplaySettings);
}

void RomBrowserController::HideDisplaySettings()
{
    if (_saveSettingsPending)
    {
        _saveSettingsPending = false;
        _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
        {
            _appSettingsService->Save();
            return TaskResult<void>::Completed();
        });
    }
    _stateMachine.Fire(RomBrowserStateTrigger::HideDisplaySettings);
}

void RomBrowserController::SetRomBrowserDisplaySettings(
    const RomBrowserDisplaySettings& romBrowserDisplaySettings)
{
    _appSettingsService->GetAppSettings().romBrowserDisplaySettings = romBrowserDisplaySettings;
    _saveSettingsPending = true;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::Update()
{
    _stateMachine.Update();
    if (_stateMachine.HasStateChanged())
    {
        HandleTrigger();
    }
    switch (_stateMachine.GetCurrentState())
    {
        case RomBrowserState::Start:
        {
            LOG_DEBUG("RomBrowserState::Start\n");
            const auto& lastUsed = _appSettingsService->GetAppSettings().lastUsedFilePath;
            if (strlen(lastUsed.GetString()) != 0)
            {
                NavigateToPath(lastUsed.GetString());
            }
            else
            {
                NavigateToPath("/");
            }
            break;
        }
        case RomBrowserState::LoadingFolder:
        {
            if (_navigateTask.GetTask().IsCompletedSuccessfully())
            {
                _navigateTask.Dispose();
                _stateMachine.Fire(RomBrowserStateTrigger::FolderLoadDone);
            }
            break;
        }
        case RomBrowserState::Launching:
        default:
        {
            break;
        }
    }
}

void RomBrowserController::HandleTrigger()
{
    switch (_stateMachine.GetLastTrigger())
    {
        case RomBrowserStateTrigger::Navigate:
            HandleNavigateTrigger();
            break;

        case RomBrowserStateTrigger::FolderLoadDone:
            HandleFolderLoadDoneTrigger();
            break;

        case RomBrowserStateTrigger::Launch:
            HandleLaunchTrigger();
            break;

        case RomBrowserStateTrigger::ChangeDisplayMode:
            HandleChangeDisplayModeTrigger();
            break;

        default:
            break;
    }
}

void RomBrowserController::HandleNavigateTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Navigate\n");
    _navigateTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        if (!_coverRepository)
        {
            _coverRepository = std::make_unique<CoverRepository>();
            _coverRepository->Initialize();
        }
        if (!_cheatRepository)
        {
            _cheatRepository = UsrCheatRepositoryFactory().FromUsrCheatDat("/_pico/usrcheat.dat");
            if (!_cheatRepository)
            {
                // When usrcheat.dat is not found or cannot be read use a dummy empty cheat repository
                _cheatRepository = std::make_unique<EmptyCheatRepository>();
            }
        }

        _navigateFileName = nullptr;
        if (!strcmp(_navigatePath, "recent:") || !strcmp(_navigatePath, "favorites:"))
        {
            // virtual folder built from a stored list of full file paths
            _virtualFolderKind = _navigatePath[0] == 'r'
                ? VirtualFolderKind::Recent : VirtualFolderKind::Favorites;
            PathListStore& store = _virtualFolderKind == VirtualFolderKind::Recent
                ? _recentStore : _favoritesStore;
            store.Load();
            _favoritesStore.Load(); // also needed for the heart indicator
            SdFolderFactory sdFolderFactory { &_fileTypeProvider };
            _newSdFolder = sdFolderFactory.CreateFromPathList(store);
            return TaskResult<void>::Completed();
        }
        _virtualFolderKind = VirtualFolderKind::None;
        _favoritesStore.Load(); // no-op after the first navigation

        u64 startTick = gTickCounter.GetValue();
        if (strcmp(_navigatePath, "/") != 0) // can't f_stat on root dir
        {
            FILINFO fileInfo;
            if (f_stat(_navigatePath, &fileInfo) != FR_OK)
            {
                StringUtil::Copy(_navigatePath, "/", sizeof(_navigatePath) / sizeof(_navigatePath[0]));
            }
            else if (!(fileInfo.fattrib & AM_DIR))
            {
                _navigateFileName = strrchr(_navigatePath, '/') + 1;
                _navigateFileName[-1] = 0;
            }
        }
        f_chdir(_navigatePath);
        f_getcwd(_currentRealPath, sizeof(_currentRealPath) / sizeof(_currentRealPath[0]));
        SdFolderFactory sdFolderFactory { &_fileTypeProvider };
        _newSdFolder = sdFolderFactory.CreateFromPath(".");
        u64 endTick = gTickCounter.GetValue();
        LOG_DEBUG("Loading files in folder took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleFolderLoadDoneTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::FolderLoadDone\n");
    _romBrowserViewModel.Reset();
    _sdFolder = std::move(_newSdFolder);
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this, _navigateFileName);
}

void RomBrowserController::HandleLaunchTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Launch\n");
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        if (!UpdateLastUsedFilepath())
        {
            LOG_ERROR("Failed to resolve full path of launched file.\n");
            return TaskResult<void>::Completed();
        }
        SetPicoLoaderParams();
        LoadCheats();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::CycleTheme(int direction)
{
_ioTaskQueue->Enqueue([this, direction] (const vu8& cancelRequested)
{
constexpr int kMaxThemes = 32;
constexpr int kNameLength = 64;
std::unique_ptr<char[]> names(new char[kMaxThemes * kNameLength]);
int count = 0;
DIR dir;
if (f_opendir(&dir, "/_pico/themes") == FR_OK)
{
FILINFO entry;
while (count < kMaxThemes && f_readdir(&dir, &entry) == FR_OK && entry.fname[0])
{
if (!(entry.fattrib & AM_DIR))
    continue;
StringUtil::Copy(&names[count * kNameLength], entry.fname, kNameLength);
count++;
}
f_closedir(&dir);
}
if (count == 0)
{
return TaskResult<void>::Completed();
}
// sort by name for a stable cycle order
for (int i = 0; i < count - 1; i++)
{
for (int j = 0; j < count - 1 - i; j++)
{
if (strcasecmp(&names[j * kNameLength], &names[(j + 1) * kNameLength]) > 0)
{
char tmp[kNameLength];
StringUtil::Copy(tmp, &names[j * kNameLength], kNameLength);
StringUtil::Copy(&names[j * kNameLength], &names[(j + 1) * kNameLength], kNameLength);
StringUtil::Copy(&names[(j + 1) * kNameLength], tmp, kNameLength);
}
}
}
const char* current = _appSettingsService->GetAppSettings().theme.GetString();
int currentIdx = 0;
for (int i = 0; i < count; i++)
{
if (!strcasecmp(&names[i * kNameLength], current))
{
currentIdx = i;
break;
}
}
int nextIdx = (currentIdx + direction + count) % count;
_appSettingsService->GetAppSettings().theme = &names[nextIdx * kNameLength];
    // remember the folder currently being browsed so the relaunch returns here
    TCHAR currentPath[256];
    f_getcwd(currentPath, sizeof(currentPath) / sizeof(currentPath[0]));
    _appSettingsService->GetAppSettings().lastUsedFilePath = currentPath;
_appSettingsService->Save();
// relaunch the launcher to apply the new theme
auto loadParams = pload_getLoadParams();
loadParams->savePath[0] = 0;
loadParams->arguments[0] = 0;
loadParams->argumentsLength = 0;
const char* launcherPath = pload_getLauncherPath();
if (!launcherPath || launcherPath[0] == 0)
{
launcherPath = "/_picoboot.nds";
}
StringUtil::Copy(loadParams->romPath, launcherPath, sizeof(loadParams->romPath));
gProcessManager.Goto<PicoLoaderProcess>();
return TaskResult<void>::Completed();
});
}

void RomBrowserController::HandleChangeDisplayModeTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::ChangeDisplayMode\n");
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this);
}

bool RomBrowserController::UpdateLastUsedFilepath()
{
    if (!ResolveItemFullPath(_triggerFileInfo.GetFileName(),
            _triggerFileInfo.GetFastFileRef().GetStartCluster(), _navigatePath,
            sizeof(_navigatePath) / sizeof(_navigatePath[0])))
    {
        // guard: never save a virtual folder path as lastUsedFilePath
        return false;
    }
    _appSettingsService->GetAppSettings().lastUsedFilePath = _navigatePath;
    _recentStore.AddFront(_navigatePath);
    _appSettingsService->Save();
    return true;
}

bool RomBrowserController::ResolveItemFullPath(const char* fileName, u32 startCluster, TCHAR* path, u32 pathLength)
{
    if (_virtualFolderKind != VirtualFolderKind::None)
    {
        PathListStore& store = _virtualFolderKind == VirtualFolderKind::Recent
            ? _recentStore : _favoritesStore;
        store.Load();
        // verify candidates by start cluster so identical file names in
        // different folders can't resolve to the wrong game
        int firstMatch = -1;
        FILINFO fileInfo;
        for (int index = store.IndexOfFileName(fileName); index >= 0;
             index = store.IndexOfFileName(fileName, index + 1))
        {
            if (firstMatch < 0)
            {
                firstMatch = index;
            }
            if (f_stat(store.GetPath(index), &fileInfo) == FR_OK &&
                fileInfo.fclust == startCluster)
            {
                StringUtil::Copy(path, store.GetPath(index), pathLength);
                return true;
            }
        }
        if (firstMatch < 0)
        {
            return false;
        }
        // file changed on disk since the list was built; fall back to the name match
        StringUtil::Copy(path, store.GetPath(firstMatch), pathLength);
        return true;
    }
    JoinPath(_currentRealPath, fileName, path, pathLength);
    return true;
}

void RomBrowserController::JoinPath(const TCHAR* dir, const char* fileName, TCHAR* path, u32 pathLength)
{
    StringUtil::Copy(path, dir, pathLength);
    int idx = strlcat(path, "/", pathLength);
    if (idx >= 2 && path[idx - 2] == '/')
    {
        path[idx - 1] = 0;
    }
    strlcat(path, fileName, pathLength);
}

bool RomBrowserController::IsFavorite(const FileInfo& fileInfo) const
{
    // Display-only check that runs on the UI thread every frame: it only reads
    // the in-memory list and never touches the sd card.
    if (!_favoritesStore.IsLoaded())
    {
        return false;
    }
    if (_virtualFolderKind != VirtualFolderKind::None)
    {
        return _favoritesStore.IndexOfFileName(fileInfo.GetFileName()) >= 0;
    }
    TCHAR path[256];
    JoinPath(_currentRealPath, fileInfo.GetFileName(), path, sizeof(path) / sizeof(path[0]));
    return _favoritesStore.Contains(path);
}

void RomBrowserController::ToggleFavorite(const FileInfo& fileInfo)
{
    if (_favoriteTogglePending)
    {
        // the previous toggle hasn't been written yet; ignore repeated presses
        // (_favoriteToggleFileInfo must not change while the io task reads it)
        return;
    }
    _favoriteTogglePending = true;
    _favoriteToggleFileInfo = FileInfo(fileInfo);
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        TCHAR path[256];
        if (ResolveItemFullPath(_favoriteToggleFileInfo.GetFileName(),
                _favoriteToggleFileInfo.GetFastFileRef().GetStartCluster(),
                path, sizeof(path) / sizeof(path[0])))
        {
            _favoritesStore.Toggle(path);
        }
        _favoriteTogglePending = false;
        return TaskResult<void>::Completed();
    });
    if (_virtualFolderKind == VirtualFolderKind::Favorites)
    {
        // refresh the favorites view so a removed item disappears immediately
        NavigateToPath("favorites:");
    }
}

void RomBrowserController::SetPicoLoaderParams() const
{
    auto loadParams = pload_getLoadParams();
    loadParams->savePath[0] = 0;
    loadParams->arguments[0] = 0;
    loadParams->argumentsLength = 0;
    if (_triggerFileInfo.GetFileType()->TrySetLaunchParameters(loadParams, _navigatePath))
    {
        gProcessManager.Goto<PicoLoaderProcess>();
    }
    else
    {
        LOG_FATAL("Failed to set launch parameters.\n");
    }
}

void RomBrowserController::LoadCheats() const
{
    auto cheats = _cheatRepository->GetCheatsForGame(_triggerFileInfo.GetFastFileRef());
    auto cheatData = PicoLoaderCheatDataFactory().CreateCheatData(cheats);
    pload_setCheatData(cheatData);
}
