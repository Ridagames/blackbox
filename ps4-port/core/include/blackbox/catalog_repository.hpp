#pragma once
#include "blackbox/catalog_entry.hpp"
#include <string>
#include <vector>
namespace blackbox {
struct CatalogResult {
    bool ok = false;
    std::string error;
    std::vector<CatalogEntry> entries;
};
// Reads metadata only; remote package/source URLs are intentionally ignored.
CatalogResult parse_catalog(const std::string& json_text);
} // namespace blackbox
