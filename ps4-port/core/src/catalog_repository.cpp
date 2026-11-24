#include "blackbox/catalog_repository.hpp"
#include <exception>
#include <nlohmann/json.hpp>
namespace blackbox {
namespace {
std::string read_string(const nlohmann::json& item, const char* key) {
    const auto it = item.find(key);
    if (it == item.end() || it->is_null() || !it->is_string()) return {};
    return it->get<std::string>();
}
std::uint64_t read_size(const nlohmann::json& item) {
    const auto it = item.find("sizeBytes");
    if (it == item.end() || !it->is_number_integer()) return 0;
    try {
        const auto value = it->get<std::int64_t>();
        return value > 0 ? static_cast<std::uint64_t>(value) : 0;
    } catch (...) { return 0; }
}
}
CatalogResult parse_catalog(const std::string& json_text) {
    CatalogResult result;
    try {
        const auto root = nlohmann::json::parse(json_text);
        if (!root.is_object()) {
            result.error = "Catalogue root must be a JSON object.";
            return result;
        }
        const auto releases = root.find("releases");
        if (releases == root.end() || !releases->is_array()) {
            result.error = "Missing or invalid releases array.";
            return result;
        }
        for (const auto& item : *releases) {
            if (!item.is_object() || read_string(item, "platform") != "ps4") continue;
            CatalogEntry entry;
            entry.id = read_string(item, "id");
            entry.title_id = read_string(item, "titleId");
            entry.title = read_string(item, "title");
            entry.platform = "ps4";
            entry.region = read_string(item, "region");
            entry.version = read_string(item, "version");
            entry.size_bytes = read_size(item);
            entry.cover = read_string(item, "cover");
            entry.hero = read_string(item, "hero");
            if (!entry.id.empty() && !entry.title_id.empty() && !entry.title.empty())
                result.entries.push_back(std::move(entry));
        }
        result.ok = true;
    } catch (const std::exception& error) {
        result.error = std::string("Invalid catalogue JSON: ") + error.what();
    }
    return result;
}
} // namespace blackbox
