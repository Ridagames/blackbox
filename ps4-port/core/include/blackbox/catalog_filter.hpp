#pragma once
#include "blackbox/catalog_entry.hpp"
#include <string>
#include <vector>
namespace blackbox {
std::vector<CatalogEntry> filter_catalog(
    const std::vector<CatalogEntry>& entries,
    const std::string& query,
    const std::string& region = {});
} // namespace blackbox
