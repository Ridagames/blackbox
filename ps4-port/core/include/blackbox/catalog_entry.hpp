#pragma once
#include <cstdint>
#include <string>
namespace blackbox {
struct CatalogEntry {
    std::string id, title_id, title, platform, region, version;
    std::uint64_t size_bytes = 0;
    std::string cover, hero;
};
} // namespace blackbox
