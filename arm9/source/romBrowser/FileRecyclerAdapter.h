#pragma once
#include "core/task/TaskQueue.h"
#include "gui/views/RecyclerAdapter.h"

class FileInfoManager;
class IVramManager;
class InternalFileInfo;
class IThemeFileIconFactory;
class VramContext;
class IRomBrowserController;

class FileRecyclerAdapter : public RecyclerAdapter
{
public:
    u32 GetItemCount() const override;
    void BindView(SharedPtr<View> view, int index) const override;
const char* GetItemSortKey(int index) const override;

    void SetIconFrameCounter(u32 iconFrameCounter)
    {
        _iconFrameCounter = iconFrameCounter;
    }

    virtual void InitVram(const VramContext& vramContext) { }

    /// @brief Whether binding an item should also load its hero image. Heroes
    ///        are only shown in cover flow mode, so loading them for grid/list
    ///        binds would waste sd-card io on every scroll.
    virtual bool LoadsHeroes() const { return false; }

protected:
    IRomBrowserController* _romBrowserController;
    FileInfoManager* _fileInfoManager;
    TaskQueueBase* _taskQueue;
    u32 _iconFrameCounter;
    const IThemeFileIconFactory* _themeFileIconFactory;

    FileRecyclerAdapter(IRomBrowserController* romBrowserController, FileInfoManager* fileInfoManager,
        TaskQueueBase* taskQueue, const IThemeFileIconFactory* themeFileIconFactory)
        : _romBrowserController(romBrowserController), _fileInfoManager(fileInfoManager), _taskQueue(taskQueue)
        , _iconFrameCounter(0), _themeFileIconFactory(themeFileIconFactory) { }

    virtual TaskResult<void> BindView(SharedPtr<View> view, int index,
        const InternalFileInfo* internalFileInfo, const vu8& cancelRequested) const = 0;
    virtual void SetQueueTask(const SharedPtr<View>& view, QueueTask<void> queueTask) const = 0;
};
