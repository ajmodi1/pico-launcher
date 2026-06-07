#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/BmpFileCover.h"
#include "FileType/InternalFileInfo.h"
#include "SdFolderFactory.h"
#include "CoverRepository.h"

static std::unique_ptr<SdFolder> LoadCoverFolder(const char* path)
{
            NullFileTypeProvider fileTypeProvider;
    auto folder = SdFolderFactory(&fileTypeProvider).CreateFromPath(path);
    if (folder)
{
        folder->SortByNameInPlace();
}
    return folder;
}

static const FileInfo* FindBmpByName(const SdFolder* folder, const char* name, char* nameBuffer, u32 bufferSize)
{
            if (!folder)
                            return nullptr;
    u32 length = StringUtil::Copy(nameBuffer, name, bufferSize - 5);
    nameBuffer[length + 0] = '.';
    nameBuffer[length + 1] = 'b';
    nameBuffer[length + 2] = 'm';
    nameBuffer[length + 3] = 'p';
    nameBuffer[length + 4] = 0;
    return folder->BinarySearch(nameBuffer);
}

void CoverRepository::Initialize()
{
            // Intentionally empty. Cover folders are loaded lazily on first use to
    // keep startup fast and the UI responsive.
}

const SdFolder* CoverRepository::GetUserCoversFolder() const
{
            if (!_userCoversLoaded)
{
        _userCoversLoaded = true;
        _userCoversFolder = LoadCoverFolder("/_pico/covers/user");
}
    return _userCoversFolder.get();
}

const SdFolder* CoverRepository::GetUserHeroesFolder() const
{
            if (!_userHeroesLoaded)
{
        _userHeroesLoaded = true;
        _userHeroesFolder = LoadCoverFolder("/_pico/heroes/user");
}
    return _userHeroesFolder.get();
}

FileCover* CoverRepository::GetHeroForFile(const FileInfo& fileInfo) const
{
            char nameBuffer[256];
    const FileInfo* heroFile = FindBmpByName(
        GetUserHeroesFolder(), fileInfo.GetFileName(), nameBuffer, sizeof(nameBuffer));
    if (heroFile)
{
        return new BmpFileCover(heroFile->GetFastFileRef());
}
    return nullptr;
}

FileCover* CoverRepository::GetCoverForFile(const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const
{
            char nameBuffer[256];
    const auto& fileType = fileInfo.GetFileType();

    // Try to get a cover based on the filename in the user folder.
    // This also applies to folders, so that folders can be given custom covers.
    const FileInfo* coverFile = FindBmpByName(
        GetUserCoversFolder(), fileInfo.GetFileName(), nameBuffer, sizeof(nameBuffer));

    if (fileType->GetClassification() != FileTypeClassification::Folder)
{
        // Try to get a cover based on an internal game code
        if (!coverFile && internalFileInfo)
{
            const auto* coverFolder = GetCoverFolder(fileType->GetShortName());
            if (coverFolder)
{
                const char* gameCode = internalFileInfo->GetGameCode();
                if (gameCode)
{
                    coverFile = FindBmpByName(coverFolder, gameCode, nameBuffer, sizeof(nameBuffer));
}
}
}

        if (coverFile)
{
            return new BmpFileCover(coverFile->GetFastFileRef());
}

        if (!coverFile && internalFileInfo)
{
            auto cover = internalFileInfo->CreateGameCover();
            if (cover)
{
                return cover;
}
}
}
    else if (coverFile)
{
        return new BmpFileCover(coverFile->GetFastFileRef());
}

    return fileType->CreateFileCover(fileInfo.GetFileName());
}

const SdFolder* CoverRepository::GetCoverFolder(const char* coverFolderName) const
{
            if (!strcmp(coverFolderName, "nds"))
{
        if (!_ndsCoversLoaded)
{
            _ndsCoversLoaded = true;
            _ndsCoversFolder = LoadCoverFolder("/_pico/covers/nds");
}
        return _ndsCoversFolder.get();
}
    else if (!strcmp(coverFolderName, "gba"))
{
        if (!_gbaCoversLoaded)
{
            _gbaCoversLoaded = true;
            _gbaCoversFolder = LoadCoverFolder("/_pico/covers/gba");
}
        return _gbaCoversFolder.get();
}
    else
{
        return nullptr;
}
}
