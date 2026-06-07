#pragma once
#include <memory>
#include "SdFolder.h"
#include "FileType/IFileTypeProvider.h"

class PathListStore;

class SdFolderFactory
{
public:
    SdFolderFactory(const IFileTypeProvider* fileTypeProvider)
        : _fileTypeProvider(fileTypeProvider) { }

    std::unique_ptr<SdFolder> CreateFromPath(const char* path) const;

    /// @brief Creates a virtual SdFolder from a list of full file paths.
    ///        Paths that no longer exist on the sd card are skipped.
    std::unique_ptr<SdFolder> CreateFromPathList(const PathListStore& pathList) const;

private:
    const IFileTypeProvider* _fileTypeProvider;
};