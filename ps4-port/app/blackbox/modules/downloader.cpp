#include "downloader.hpp"
#include <orbis/Http.h>
#include <orbis/Net.h>
#include <orbis/Ssl.h>
#include <orbis/Sysmodule.h>
#include <orbis/_types/http.h>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <string>

namespace blackbox {
namespace {
constexpr int kNetPoolSize = 256 * 1024;
constexpr const char* kUserAgent = "BLACKBOX-PS4/0.2 (OpenOrbis)";

bool has_pkg_extension(const std::string& value) {
    if (value.size() < 4) return false;
    std::string ext = value.substr(value.size() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".pkg";
}
bool allowed_url(const std::string& url) {
    return url.rfind("https://github.com/", 0) == 0;
}
struct HttpState {
    int net_pool = -1;
    bool net_started = false;
    int ssl_context = -1;
    int http_context = -1;
    int template_id = -1;
    int connection_id = -1;
    int request_id = -1;
};
void cleanup(HttpState& s) {
    if (s.request_id >= 0) sceHttpDeleteRequest(s.request_id);
    if (s.connection_id >= 0) sceHttpDeleteConnection(s.connection_id);
    if (s.template_id >= 0) sceHttpDeleteTemplate(s.template_id);
    if (s.http_context >= 0) sceHttpTerm(s.http_context);
    if (s.ssl_context >= 0)
        reinterpret_cast<int32_t(*)(int32_t)>(&sceSslTerm)(s.ssl_context);
    if (s.net_pool >= 0) sceNetPoolDestroy(s.net_pool);
    if (s.net_started) sceNetTerm();
}
bool initialize(HttpState& s) {
    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET) < 0 ||
        sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP) < 0 ||
        sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL) < 0)
        return false;
    if (sceNetInit() < 0) return false;
    s.net_started = true;
    s.net_pool = sceNetPoolCreate("BlackBoxNet", kNetPoolSize, 0);
    if (s.net_pool < 0) return false;
    s.ssl_context = sceSslInit(SSL_POOLSIZE);
    if (s.ssl_context < 0) return false;
    s.http_context = sceHttpInit(s.net_pool, s.ssl_context, LIBHTTP_POOLSIZE);
    return s.http_context >= 0;
}
void report(const DownloadCallback& cb, DownloadStatus& status) {
    if (cb) cb(status);
}
}

bool download_pkg(const DownloadEntry& entry, DownloadCallback callback) {
    DownloadStatus status;
    // Fail closed: only a curated, explicitly marked homebrew entry can download.
    if (entry.commercial || !entry.homebrew) {
        status.error = "Commercial content blocked";
        status.finished = true;
        report(callback, status);
        return false;
    }
    if (!has_pkg_extension(entry.filename) ||
        !has_pkg_extension(entry.output_path) ||
        !allowed_url(entry.url)) {
        status.error = "Invalid or untrusted PKG source";
        status.finished = true;
        report(callback, status);
        return false;
    }

    HttpState state;
    if (!initialize(state)) {
        cleanup(state);
        status.error = "PS4 network services could not start";
        status.finished = true;
        report(callback, status);
        return false;
    }

    state.template_id = sceHttpCreateTemplate(
        state.http_context, kUserAgent, ORBIS_HTTP_VERSION_1_1, 1);
    if (state.template_id < 0) {
        status.error = "Could not create HTTPS session";
        cleanup(state);
        status.finished = true;
        report(callback, status);
        return false;
    }

    // Keep default certificate validation enabled and allow HTTPS redirects.
    using RedirectFn = int32_t(*)(int32_t, int32_t);
    const int redirect_result =
        reinterpret_cast<RedirectFn>(&sceHttpSetAutoRedirect)(state.template_id, 1);
    if (redirect_result < 0) {
        status.error = "Could not enable secure redirects";
        cleanup(state);
        status.finished = true;
        report(callback, status);
        return false;
    }
    sceHttpSetConnectTimeOut(state.template_id, 30000000);
    sceHttpSetSendTimeOut(state.template_id, 30000000);
    sceHttpSetRecvTimeOut(state.template_id, 60000000);

    state.connection_id = sceHttpCreateConnectionWithURL(
        state.template_id, entry.url.c_str(), true);
    if (state.connection_id < 0) {
        status.error = "Could not connect to PKG source";
        cleanup(state);
        status.finished = true;
        report(callback, status);
        return false;
    }
    state.request_id = sceHttpCreateRequestWithURL(
        state.connection_id, ORBIS_METHOD_GET, entry.url.c_str(), 0);
    if (state.request_id < 0 ||
        sceHttpSendRequest(state.request_id, nullptr, 0) < 0) {
        status.error = "PKG request failed";
        cleanup(state);
        status.finished = true;
        report(callback, status);
        return false;
    }

    int32_t http_status = 0;
    if (sceHttpGetStatusCode(state.request_id, &http_status) < 0 ||
        http_status != 200) {
        status.error = "Source returned HTTP " + std::to_string(http_status);
        cleanup(state);
        status.finished = true;
        report(callback, status);
        return false;
    }

    int content_length_type = ORBIS_HTTP_CONTENTLEN_NOT_FOUND;
    std::size_t content_length = 0;
    if (sceHttpGetResponseContentLength(
            state.request_id, &content_length_type, &content_length) == 0 &&
        content_length_type == ORBIS_HTTP_CONTENTLEN_EXIST)
        status.total = static_cast<std::uint64_t>(content_length);

    FILE* file = std::fopen(entry.output_path.c_str(), "wb");
    if (!file) {
        status.error = "Cannot create destination file";
        cleanup(state);
        status.finished = true;
        report(callback, status);
        return false;
    }

    unsigned char buffer[64 * 1024];
    bool ok = true;
    for (;;) {
        const int received = sceHttpReadData(state.request_id, buffer, sizeof(buffer));
        if (received == 0) break;
        if (received < 0 || std::fwrite(buffer, 1, static_cast<std::size_t>(received), file)
                               != static_cast<std::size_t>(received)) {
            ok = false;
            break;
        }
        status.downloaded += static_cast<std::uint64_t>(received);
        report(callback, status);
    }
    if (std::fclose(file) != 0) ok = false;
    cleanup(state);

    status.finished = true;
    status.success = ok && status.downloaded > 0 &&
        (status.total == 0 || status.downloaded == status.total);
    if (!status.success) {
        std::remove(entry.output_path.c_str());
        if (status.error.empty()) status.error = "PKG transfer incomplete";
    }
    report(callback, status);
    return status.success;
}
}
