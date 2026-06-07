#pragma once

enum class SdFolderSortType
{
    Name,
    LastModified,

    /// @brief Keeps the original order of the folder (used for virtual
    ///        folders such as recently played, which are pre-ordered).
    None
};
