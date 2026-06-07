#pragma once
#include <stdio.h>
#include "FileType.h"
#include "services/settings/FileAssociation.h"
#include "../Theme/IThemeFileIconFactory.h"
#include "BmpFileIcon.h"

/// @brief Class representing a custom (user provided) file type.
class CustomFileType : public FileType
{
public:
    CustomFileType()
        : FileType(nullptr, FileTypeClassification::Unknown) { }

    explicit CustomFileType(const FileAssociation* fileAssociation)
        : CustomFileType(fileAssociation, nullptr) { }

    CustomFileType(const FileAssociation* fileAssociation, const FileType* baseFileType)
        : FileType(
            baseFileType != nullptr ? baseFileType->GetShortName() : fileAssociation->extension.GetString(),
            baseFileType != nullptr ? baseFileType->GetClassification() : FileTypeClassification::Misc)
        , _fileAssociation(fileAssociation), _baseFileType(baseFileType) { }

    std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const override
{
            // Try a custom per-game icon from /_pico/icons/<type>/<fileName>.bmp
        // (for example /_pico/icons/gbc/Mario Tennis.gbc.bmp)
        if (fileName && fileName[0] && GetShortName() != nullptr)
{
            char path[300];
            int length = snprintf(path, sizeof(path), "/_pico/icons/%s/%s.bmp", GetShortName(), fileName);
            if (length > 0 && length < (int)sizeof(path))
{
                auto icon = BmpFileIcon::TryCreateFromPath(path);
                if (icon)
                                        return icon;
}
}

        return _baseFileType != nullptr
                        ? _baseFileType->CreateFileIcon(fileName, themeFileIconFactory)
                        : (themeFileIconFactory != nullptr
                            ? themeFileIconFactory->CreateGenericFileIcon(fileName)
                            : nullptr);
}

    FileCover* CreateFileCover(const TCHAR* fileName) const override
{
            return _baseFileType != nullptr
                            ? _baseFileType->CreateFileCover(fileName)
                            : FileType::CreateFileCover(fileName);
};

    bool HasInternalFileInfo() const override
{
            return _baseFileType != nullptr
                            && _baseFileType->HasInternalFileInfo();
}

    InternalFileInfo* CreateInternalFileInfo(const FastFileRef& fastFileRef) const override
{
            return _baseFileType != nullptr
                            ? _baseFileType->CreateInternalFileInfo(fastFileRef)
                            : nullptr;
}

    bool TrySetLaunchParameters(pload_params_t* launchParameters, const char* filePath) const override
{
            StringUtil::Copy(launchParameters->romPath, _fileAssociation->applicationPath, sizeof(launchParameters->romPath));
        u32 length = StringUtil::Copy(launchParameters->arguments, filePath, sizeof(launchParameters->arguments));
        launchParameters->argumentsLength = length + 1;
        return true;
}

private:
    const FileAssociation* _fileAssociation;
    const FileType* _baseFileType = nullptr;
};
