#pragma once
#include "common.h"
#include <memory>
#include "FileInfo.h"
#include "FileType/FileCover.h"
#include "ICoverRepository.h"
#include "core/AtomicSharedPtr.h"
#include "FileType/InternalFileInfo.h"

class FileInfoManager
{
public:
    FileInfoManager(std::unique_ptr<const FileInfo*[]> items, u32 itemCount, const ICoverRepository& coverRepository);
    ~FileInfoManager();

    const InternalFileInfo* GetInternalFileInfo(int index)
{
            return _extraFileInfo[index].internalFileInfo;
}

    SharedPtr<FileCover> GetFileCover(int index)
{
            return _extraFileInfo[index].fileCover.Lock();
}

    SharedPtr<FileCover> GetFileHero(int index)
{
            return _extraFileInfo[index].fileHero.Lock();
}

    /// @brief Loads the internal file info and cover for the given item.
    /// @param loadHero Also load the hero image. Heroes are only displayed in
    ///        cover flow mode, so grid/list binds skip them to halve the sd-card
    ///        io per item while scrolling.
    void LoadFileInfo(int index, bool loadHero = true);

    void ReleaseFileInfo(int index);

    int GetItemIndex(const char* fileName);

    const FileInfo& GetItem(int index) const { return *_items[index]; }
    u32 GetItemCount() const { return _itemCount; }

private:
    struct ExtraFileInfo
{
        const InternalFileInfo* internalFileInfo;
        AtomicSharedPtr<FileCover> fileCover;
        AtomicSharedPtr<FileCover> fileHero;
};

    std::unique_ptr<const FileInfo*[]> _items;
    u32 _itemCount;
    std::unique_ptr<ExtraFileInfo[]> _extraFileInfo;
    const ICoverRepository& _coverRepository;
};
