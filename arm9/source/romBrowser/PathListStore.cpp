#include "common.h"
#include <memory>
#include <string.h>
#include "json/ArduinoJson.h"
#include "fat/File.h"
#include "core/StringUtil.h"
#include "PathListStore.h"

#define JSON_RESERVED_SIZE  16384

void PathListStore::Load()
{
    if (_loadStarted)
        return;
    _loadStarted = true;
    _paths = std::make_unique<char[]>(_maxEntries * kPathLength);
    _count = 0;
    ReadFromFile();
    // set last so UI thread readers gated on IsLoaded() never see a partial list
    _loaded = true;
}

void PathListStore::ReadFromFile()
{
    const auto file = std::make_unique<File>();
    if (file->Open(_filePath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return;

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u32 bytesRead = 0;
    if (file->Read(fileData.get(), fileSize, bytesRead) != FR_OK)
        return;

    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileData.get(), fileSize) != DeserializationError::Ok)
        return;

    for (JsonVariantConst entry : json.as<JsonArrayConst>())
    {
        const char* path = entry.as<const char*>();
        if (!path || !path[0] || _count >= _maxEntries)
            continue;
        StringUtil::Copy(&_paths[_count * kPathLength], path, kPathLength);
        _count++;
    }
}

void PathListStore::Save() const
{
    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    auto jsonArray = json.to<JsonArray>();
    for (int i = 0; i < _count; i++)
    {
        jsonArray.add((const char*)&_paths[i * kPathLength]);
    }

    u32 outputSize = measureJsonPretty(json);
    std::unique_ptr<u8[]> fileData(new(cache_align) u8[outputSize]);
    serializeJsonPretty(json, fileData.get(), outputSize);

    const auto file = std::make_unique<File>();
    if (file->Open(_filePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Couldn't open path list file for writing\n");
        return;
    }
    u32 bytesWritten;
    if (file->Write(fileData.get(), outputSize, bytesWritten) != FR_OK || bytesWritten != outputSize)
    {
        LOG_ERROR("Error while writing path list file\n");
    }
}

int PathListStore::IndexOf(const char* path) const
{
    for (int i = 0; i < _count; i++)
    {
        if (!strcasecmp(&_paths[i * kPathLength], path))
            return i;
    }
    return -1;
}

int PathListStore::IndexOfFileName(const char* fileName, int fromIndex) const
{
    if (!fileName)
        return -1;
    for (int i = fromIndex; i < _count; i++)
    {
        const char* entry = &_paths[i * kPathLength];
        const char* slash = strrchr(entry, '/');
        const char* entryName = slash ? slash + 1 : entry;
        if (!strcasecmp(entryName, fileName))
            return i;
    }
    return -1;
}

void PathListStore::RemoveAt(int index)
{
    for (int i = index; i < _count - 1; i++)
    {
        StringUtil::Copy(&_paths[i * kPathLength], &_paths[(i + 1) * kPathLength], kPathLength);
    }
    _count--;
}

void PathListStore::AddFront(const char* path)
{
    Load();
    int existing = IndexOf(path);
    if (existing == 0)
        return; // already at the front
    if (existing > 0)
        RemoveAt(existing);
    if (_count >= _maxEntries)
        _count = _maxEntries - 1; // drop the oldest entry
    for (int i = _count; i > 0; i--)
    {
        StringUtil::Copy(&_paths[i * kPathLength], &_paths[(i - 1) * kPathLength], kPathLength);
    }
    StringUtil::Copy(&_paths[0], path, kPathLength);
    _count++;
    Save();
}

bool PathListStore::Toggle(const char* path)
{
    Load();
    int existing = IndexOf(path);
    if (existing >= 0)
    {
        RemoveAt(existing);
        Save();
        return false;
    }
    if (_count >= _maxEntries)
        return false; // list is full
    StringUtil::Copy(&_paths[_count * kPathLength], path, kPathLength);
    _count++;
    Save();
    return true;
}
