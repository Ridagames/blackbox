#include "blackbox/catalog_filter.hpp"
#include <algorithm>
#include <cctype>
namespace blackbox {
namespace {
std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
}
std::vector<CatalogEntry> filter_catalog(
    const std::vector<CatalogEntry>& entries, const std::string& query, const std::string& region) {
    const auto needle = lower(query);
    const auto wanted_region = lower(region);
    std::vector<CatalogEntry> filtered;
    for (const auto& entry : entries) {
        const bool matches_query = needle.empty() ||
            lower(entry.title).find(needle) != std::string::npos ||
            lower(entry.title_id).find(needle) != std::string::npos ||
            lower(entry.id).find(needle) != std::string::npos;
        const bool matches_region = wanted_region.empty() || lower(entry.region) == wanted_region;
        if (matches_query && matches_region) filtered.push_back(entry);
    }
    return filtered;
}
} // namespace blackbox
