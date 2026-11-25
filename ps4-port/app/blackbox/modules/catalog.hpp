#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace blackbox {
struct CatalogEntry {
    std::string id;
    std::string title_id;
    std::string title;
    std::string game_id;
    std::string region;
    std::string version;
    std::string publisher;
    std::string release_date;
    std::string cover;
    std::string hero;
    std::uint64_t size_bytes = 0;
    // Fail closed: entries are commercial unless the catalogue explicitly marks them as homebrew.
    bool commercial = true;
};
class Catalog {
public:
    bool load(const std::string& path);
    const std::vector<CatalogEntry>& entries() const { return entries_; }
    const std::string& error() const { return error_; }
private:
    std::vector<CatalogEntry> entries_;
    std::string error_;
};
}
