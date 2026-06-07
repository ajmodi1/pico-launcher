#pragma once
#include "../IRomBrowserController.h"

/// @brief View model for the search bottom sheet.
class SearchViewModel
{
public:
    explicit SearchViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    /// @brief Gets the currently active search query so the sheet can prefill it.
    const char* GetInitialQuery() const
    {
        return _romBrowserController->GetSearchQuery();
    }

    /// @brief Applies the given query as the active search filter and closes the sheet.
    void Commit(const char* query)
    {
        _romBrowserController->CommitSearch(query);
    }

    /// @brief Closes the sheet without changing the active search filter.
    void Close()
    {
        _romBrowserController->HideSearch();
    }

private:
    IRomBrowserController* _romBrowserController;
};
