#pragma once
#include "core/task/TaskQueue.h"

class IRomBrowserController;

class RomBrowserItemViewModel
{
public:
    explicit RomBrowserItemViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    void Activate();
    void ShowGameInfo();

    /// @brief Returns whether the bound item is a favorited game. The result is
    ///        cached and only recomputed when the favorites list version changes,
    ///        so this is cheap enough to call every frame from Draw.
    bool IsFavorite();

    void SetIndex(int index)
    {
        if (index != _index)
        {
            _index = index;
            _checkedFavoritesVersion = 0; // invalidate the favorite cache
        }
    }

    void SetQueueTask(QueueTask<void> queueTask)
    {
        _queueTask = std::move(queueTask);
    }

    void CancelQueueTask()
    {
        _queueTask.CancelTask();
    }

    void DisposeQueueTaskWhenComplete()
    {
        if (_queueTask.GetTask().IsCompleted())
        {
            _queueTask.Dispose();
        }
    }

private:
    int _index = -1;
    QueueTask<void> _queueTask;
    bool _isFavorite = false;
    u32 _checkedFavoritesVersion = 0; // 0 = not checked (versions start at 1)

    IRomBrowserController* _romBrowserController;
};
