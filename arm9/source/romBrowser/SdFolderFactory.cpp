#include "common.h"
#include <vector>
#include <string.h>
#include "fat/Directory.h"
#include "FileInfo.h"
#include "FileType/Folder/FolderFileType.h"
#include "VBlank.h"
#include "PathListStore.h"
#include "SdFolderFactory.h"

// Number of directory entries to read before briefly yielding to lower
// priority threads (such as the UI thread). Without this, enumerating a
// large folder on the IO thread starves the UI thread and input freezes.
#define ENTRIES_PER_YIELD   48

std::unique_ptr<SdFolder> SdFolderFactory::CreateFromPath(const char* path) const
{
        Directory directory;
    if (directory.Open(path) != FR_OK)
                return nullptr;

    int count = 0;
    int bufferSize = 8;
    int entriesSinceYield = 0;
    auto fileInfos = (FileInfo**)malloc(sizeof(FileInfo*) * bufferSize);
    auto sdFileInfo = std::make_unique<FILINFO>();
    while (true)
{
        if (directory.Read(sdFileInfo.get()) != FR_OK)
                        return nullptr;

        if (sdFileInfo->fname[0] == 0)
                        break;

        if (count >= bufferSize)
{
            bufferSize *= 2;
            fileInfos = (FileInfo**)realloc(fileInfos, sizeof(FileInfo*) * bufferSize);
}
        auto fileType = sdFileInfo->fattrib & AM_DIR
            ? &FolderFileType::sInstance
            : _fileTypeProvider->GetFileType(sdFileInfo->fname);
        fileInfos[count++] = new FileInfo(sdFileInfo->fname, fileType,
                        FastFileRef(directory.GetFatFsDirectory(), sdFileInfo.get()), sdFileInfo->fattrib);

        // Periodically give the UI thread a chance to run so input stays responsive.
        if (++entriesSinceYield >= ENTRIES_PER_YIELD)
{
            entriesSinceYield = 0;
            VBlank::Wait();
}
}

    return std::make_unique<SdFolder>(fileInfos, count);
}

std::unique_ptr<SdFolder> SdFolderFactory::CreateFromPathList(const PathListStore& pathList) const
{
    int count = 0;
    int bufferSize = 8;
    auto fileInfos = (FileInfo**)malloc(sizeof(FileInfo*) * bufferSize);
    auto sdFileInfo = std::make_unique<FILINFO>();
    for (int i = 0; i < pathList.GetCount(); i++)
{
        const char* path = pathList.GetPath(i);
        if (f_stat(path, sdFileInfo.get()) != FR_OK)
            continue; // skip files that no longer exist
        if (sdFileInfo->fattrib & AM_DIR)
            continue;
        const char* slash = strrchr(path, '/');
        const char* fileName = slash ? slash + 1 : path;
        if (count >= bufferSize)
{
            bufferSize *= 2;
            fileInfos = (FileInfo**)realloc(fileInfos, sizeof(FileInfo*) * bufferSize);
}
        fileInfos[count++] = new FileInfo(fileName, _fileTypeProvider->GetFileType(fileName),
                        FastFileRef(&gFatFs, sdFileInfo.get()), sdFileInfo->fattrib);

        // Periodically give the UI thread a chance to run so input stays responsive.
        if ((i & 7) == 7)
{
            VBlank::Wait();
}
}

    return std::make_unique<SdFolder>(fileInfos, count);
}
