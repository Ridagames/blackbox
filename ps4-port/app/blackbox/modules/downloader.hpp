#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace blackbox {
struct DownloadEntry {
    std::string url;
    std::string filename;
    std::string output_path;
    bool commercial = true;
    bool homebrew = false;
};
struct DownloadStatus {
    std::uint64_t downloaded = 0;
    std::uint64_t total = 0;
    bool finished = false;
    bool success = false;
    std::string error;
};
using DownloadCallback = std::function<void(const DownloadStatus&)>;
bool download_pkg(const DownloadEntry& entry, DownloadCallback callback = nullptr);
}
