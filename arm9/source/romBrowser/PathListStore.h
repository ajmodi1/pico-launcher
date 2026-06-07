#pragma once
#include <memory>

/// @brief A persistent, ordered list of full file paths, stored on the sd card
///        as a JSON array of strings. Used for /_pico/recent.json (recently
///        played games) and /_pico/favorites.json (favorite games).
class PathListStore
{
public:
    static constexpr int kPathLength = 256;

    PathListStore(const char* filePath, int maxEntries)
        : _filePath(filePath), _maxEntries(maxEntries) { }

    /// @brief Loads the list from the sd card if it hasn't been loaded yet.
    void Load();

    /// @brief Inserts the path at the front of the list (moving it there if it
    ///        is already present) and trims the list to maxEntries. Saves the list.
    void AddFront(const char* path);

    /// @brief Adds the path when it isn't in the list, or removes it otherwise.
    ///        Saves the list.
    /// @return true when the path is in the list after toggling.
    bool Toggle(const char* path);

    /// @brief Finds the index of the entry whose file name (last path segment)
    ///        matches the given file name.
    /// @return The index of the matching entry, or -1 when not found.
    int IndexOfFileName(const char* fileName) const;

    int GetCount() const { return _count; }
    const char* GetPath(int index) const { return &_paths[index * kPathLength]; }

private:
    const char* _filePath;
    int _maxEntries;
    int _count = 0;
    bool _loaded = false;
    std::unique_ptr<char[]> _paths;

    int IndexOf(const char* path) const;
    void RemoveAt(int index);
    void Save() const;
};
