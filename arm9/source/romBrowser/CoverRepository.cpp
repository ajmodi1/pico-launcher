#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/BmpFileCover.h"
#include "FileType/InternalFileInfo.h"
#include "SdFolderFactory.h"
#include "CoverRepository.h"

void CoverRepository::Initialize()
{
    NullFileTypeProvider fileTypeProvider;
    _ndsCoversFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/covers/nds");
    if (_ndsCoversFolder)
    {
        _ndsCoversFolder->SortByNameInPlace();
    }
    _gbaCoversFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/covers/gba");
    if (_gbaCoversFolder)
    {
        _gbaCoversFolder->SortByNameInPlace();
    }
    _userCoversFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/covers/user");
    if (_userCoversFolder)
    {
        _userCoversFolder->SortByNameInPlace();
    }
}

FileCover* CoverRepository::GetCoverForFile(const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const
{
    char nameBuffer[256];
    const auto& fileType = fileInfo.GetFileType();
    const FileInfo* coverFile = nullptr;

    // Try to get a cover based on the filename in the user folder.
    // This also applies to folders, so that folders can be given custom covers.
    if (_userCoversFolder)
    {
        u32 length = StringUtil::Copy(nameBuffer, fileInfo.GetFileName(), sizeof(nameBuffer) - 5);
        nameBuffer[length + 0] = '.';
        nameBuffer[length + 1] = 'b';
        nameBuffer[length + 2] = 'm';
        nameBuffer[length + 3] = 'p';
        nameBuffer[length + 4] = 0;
        coverFile = _userCoversFolder->BinarySearch(nameBuffer);
    }

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
                    u32 length = StringUtil::Copy(nameBuffer, gameCode, sizeof(nameBuffer) - 5);
                    nameBuffer[length + 0] = '.';
                    nameBuffer[length + 1] = 'b';
                    nameBuffer[length + 2] = 'm';
                    nameBuffer[length + 3] = 'p';
                    nameBuffer[length + 4] = 0;
                }

                coverFile = coverFolder->BinarySearch(nameBuffer);
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
        return _ndsCoversFolder.get();
    }
    else if (!strcmp(coverFolderName, "gba"))
    {
        return _gbaCoversFolder.get();
    }
    else
    {
        return nullptr;
    }
}
