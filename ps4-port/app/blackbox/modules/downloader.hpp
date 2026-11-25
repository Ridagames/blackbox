#pragma once

// BlackBox PS4 - PKG downloader (homebrew/authorized files only).
// This is an isolated module: it is not wired to the UI yet.
// Requires libcurl headers and libcurl at link time.

#include <curl/curl.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <functional>
#include <algorithm>
#include <cctype>

namespace blackbox {

struct DownloadEntry {
    std::string url;
    std::string filename;
    std::string output_path;
    bool authorized_homebrew = false;
};

struct DownloadStatus {
    double progress = 0.0;
    std::uint64_t downloaded = 0;
    std::uint64_t total = 0;
    bool finished = false;
    bool success = false;
    std::string error;
};

using DownloadCallback = std::function<void(const DownloadStatus&)>;

namespace download_detail {

struct TransferContext {
    FILE* file = nullptr;
    DownloadStatus status;
    DownloadCallback callback;
};

inline bool has_pkg_extension(const std::string& value) {
    if (value.size() < 4) return false;
    std::string extension = value.substr(value.size() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".pkg";
}

inline bool is_https_url(const std::string& url) {
    return url.size() >= 8 &&
        url.compare(0, 8, "https://") == 0;
}

inline size_t write_callback(char* data, size_t size, size_t count, void* user) {
    TransferContext* ctx = static_cast<TransferContext*>(user);
    return std::fwrite(data, size, count, ctx->file);
}

inline int progress_callback(void* user, curl_off_t total, curl_off_t downloaded,
                             curl_off_t, curl_off_t) {
    TransferContext* ctx = static_cast<TransferContext*>(user);
    ctx->status.total = static_cast<std::uint64_t>(total);
    ctx->status.downloaded = static_cast<std::uint64_t>(downloaded);
    if (total > 0)
        ctx->status.progress = 100.0 * static_cast<double>(downloaded) /
                               static_cast<double>(total);
    if (ctx->callback) ctx->callback(ctx->status);
    return 0;
}

} // namespace download_detail

inline bool download_pkg(const DownloadEntry& entry,
                         DownloadCallback callback = nullptr) {
    using namespace download_detail;

    // Fail closed unless the caller has verified the file is authorized homebrew.
    if (!entry.authorized_homebrew) return false;
    if (!has_pkg_extension(entry.filename) ||
        !has_pkg_extension(entry.output_path) ||
        !is_https_url(entry.url) ||
        entry.output_path.empty()) return false;

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    TransferContext ctx;
    ctx.callback = callback;
    ctx.file = std::fopen(entry.output_path.c_str(), "wb");
    if (!ctx.file) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, entry.url.c_str());
    // CURLOPT_PROTOCOLS is retained for compatibility with older libcurl builds.
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);

    const CURLcode result = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    std::fclose(ctx.file);
    curl_easy_cleanup(curl);

    ctx.status.finished = true;
    ctx.status.success = result == CURLE_OK && http_code >= 200 && http_code < 300;
    if (!ctx.status.success) {
        std::remove(entry.output_path.c_str());
        ctx.status.error = "PKG download failed";
    }
    if (callback) callback(ctx.status);
    return ctx.status.success;
}

} // namespace blackbox
