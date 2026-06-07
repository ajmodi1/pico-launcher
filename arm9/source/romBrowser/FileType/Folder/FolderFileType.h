#pragma once
#include <memory>
#include "../FileType.h"
#include "../../Theme/IThemeFileIconFactory.h"
#include "FolderFileCover.h"

/// @brief File type that represents a folder.
class FolderFileType : public FileType
{
public:
    static const FolderFileType sInstance;

    std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const override;

    FileCover* CreateFileCover(const TCHAR* fileName) const override
    {
        return new FolderFileCover();
    };

private:
    constexpr FolderFileType()
        : FileType("folder", FileTypeClassification::Folder) { }
};
