#include "common.h"
#include <stdio.h>
#include "../BmpFileIcon.h"
#include "FolderFileType.h"

const FolderFileType FolderFileType::sInstance;

std::unique_ptr<FileIcon> FolderFileType::CreateFileIcon(const TCHAR* fileName,
    const IThemeFileIconFactory* themeFileIconFactory) const
{
    // Try to load a custom folder icon from /_pico/icons/<folderName>.bmp
    if (fileName && fileName[0])
    {
        char path[280];
        int length = snprintf(path, sizeof(path), "/_pico/icons/%s.bmp", fileName);
        if (length > 0 && length < (int)sizeof(path))
        {
            auto icon = BmpFileIcon::TryCreateFromPath(path);
            if (icon)
                return icon;
        }
    }

    return themeFileIconFactory ? themeFileIconFactory->CreateFolderIcon(fileName) : nullptr;
}
