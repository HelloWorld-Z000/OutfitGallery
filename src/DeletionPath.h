#pragma once
#include <filesystem>
#include <string>

namespace Gallery {
// Authorize the virtual name, never compare backing directories from canonical().
// MO2 can map the directory and its children to different physical mods.
inline bool IsDirectDeletionChild(const std::filesystem::path& path,
                                  const std::filesystem::path& directory) {
    const auto name=path.filename().native();
    if(name.empty() || name==L"." || name==L".." ||
       name.find_first_of(L"/\\:")!=std::wstring::npos ||
       name.back()==L'.' || name.back()==L' ') return false;
    // Reject traversal before normalization, even if it would land inside.
    for(const auto& part:path) if(part==L"..") return false;
    return std::filesystem::absolute(path).lexically_normal().parent_path()==
           std::filesystem::absolute(directory).lexically_normal();
}
}
