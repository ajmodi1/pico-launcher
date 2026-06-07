#pragma once
#include "ICoverRepository.h"
#include "SdFolder.h"

class CoverRepository : public ICoverRepository
{
public:
    void Initialize() override;
    FileCover* GetCoverForFile(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const override;
    FileCover* GetHeroForFile(const FileInfo& fileInfo) const override;

private:
    // Cover folders are loaded lazily on first use, so that the (potentially
    // long) enumeration of large cover folders does not happen during startup
    // and block the UI from becoming responsive.
    mutable bool _ndsCoversLoaded = false;
    mutable bool _gbaCoversLoaded = false;
    mutable bool _userCoversLoaded = false;
    mutable bool _userHeroesLoaded = false;
    mutable std::unique_ptr<SdFolder> _ndsCoversFolder;
    mutable std::unique_ptr<SdFolder> _gbaCoversFolder;
    mutable std::unique_ptr<SdFolder> _userCoversFolder;
    mutable std::unique_ptr<SdFolder> _userHeroesFolder;

    const SdFolder* GetUserCoversFolder() const;
    const SdFolder* GetUserHeroesFolder() const;
    const SdFolder* GetCoverFolder(const char* coverFolderName) const;
};
