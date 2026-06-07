#pragma once
#include "FileType/FileCover.h"

class FileInfo;
class InternalFileInfo;

class ICoverRepository
{
public:
    virtual ~ICoverRepository() = 0;

    virtual void Initialize() = 0;
    virtual FileCover* GetCoverForFile(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const = 0;

    /// @brief Gets the hero image for the given file, or nullptr when none exists.
    ///        Hero images are looked up by filename in /_pico/heroes/user.
    virtual FileCover* GetHeroForFile(const FileInfo& fileInfo) const = 0;
};

inline ICoverRepository::~ICoverRepository() { }
