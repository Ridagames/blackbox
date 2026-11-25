#include "catalog.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>

namespace blackbox {
namespace {
bool read_string(const std::string& object, const char* key, std::string& out) {
    const std::string marker = std::string("\"") + key + "\"";
    std::size_t pos = object.find(marker);
    if (pos == std::string::npos) return false;
    pos = object.find(':', pos + marker.size());
    if (pos == std::string::npos) return false;
    ++pos;
    while (pos < object.size() && (object[pos] == ' ' || object[pos] == '\n' || object[pos] == '\r' || object[pos] == '\t')) ++pos;
    if (pos >= object.size() || object[pos] != '"') return false;
    ++pos;
    out.clear();
    while (pos < object.size()) {
        const char c = object[pos++];
        if (c == '"') return true;
        if (c == '\\' && pos < object.size()) {
            const char escaped = object[pos++];
            switch (escaped) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'n': out.push_back(' '); break;
                case 'r': case 't': break;
                default: out.push_back(escaped); break;
            }
        } else {
            out.push_back(c);
        }
    }
    return false;
}
bool is_commercial_title_id(const std::string& id) {
    const char* prefixes[] = {"CUSA", "PPSA", "PCAS", "PLAS", "NPEA", "NPUB", "NPUA"};
    for (const char* prefix : prefixes) {
        if (id.compare(0, 4, prefix) == 0) return true;
    }
    return false;
}
bool read_boolean_false(const std::string& object, const char* key) {
    const std::string marker = std::string("\"") + key + "\"";
    std::size_t pos = object.find(marker);
    if (pos == std::string::npos) return false;
    pos = object.find(':', pos + marker.size());
    if (pos == std::string::npos) return false;
    ++pos;
    while (pos < object.size() && (object[pos] == ' ' || object[pos] == '\n' ||
           object[pos] == '\r' || object[pos] == '\t')) ++pos;
    return object.compare(pos, 5, "false") == 0;
}
std::uint64_t read_number(const std::string& object, const char* key) {
    const std::string marker = std::string("\"") + key + "\"";
    std::size_t pos = object.find(marker);
    if (pos == std::string::npos) return 0;
    pos = object.find(':', pos + marker.size());
    if (pos == std::string::npos) return 0;
    ++pos;
    while (pos < object.size() && (object[pos] == ' ' || object[pos] == '\n')) ++pos;
    return static_cast<std::uint64_t>(std::strtoull(object.c_str() + pos, nullptr, 10));
}
}

bool Catalog::load(const std::string& path) {
    entries_.clear();
    error_.clear();
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file) {
        error_ = "Catalogue file not found";
        return false;
    }
    const std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const std::size_t entries_key = json.find("\"entries\"");
    if (entries_key == std::string::npos) {
        error_ = "Catalogue entries are missing";
        return false;
    }
    const std::size_t array_start = json.find('[', entries_key);
    if (array_start == std::string::npos) {
        error_ = "Catalogue entries array is invalid";
        return false;
    }

    std::size_t pos = array_start + 1;
    while (pos < json.size()) {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t' || json[pos] == ',')) ++pos;
        if (pos >= json.size() || json[pos] == ']') break;
        if (json[pos] != '{') {
            error_ = "Catalogue entry is malformed";
            entries_.clear();
            return false;
        }
        const std::size_t start = pos;
        int depth = 0;
        bool in_string = false;
        bool escaped = false;
        for (; pos < json.size(); ++pos) {
            const char c = json[pos];
            if (in_string) {
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') in_string = false;
            } else {
                if (c == '"') in_string = true;
                else if (c == '{') ++depth;
                else if (c == '}' && --depth == 0) { ++pos; break; }
            }
        }
        if (depth != 0) {
            error_ = "Catalogue object is unterminated";
            entries_.clear();
            return false;
        }
        const std::string object = json.substr(start, pos - start);
        CatalogEntry entry;
        read_string(object, "id", entry.id);
        read_string(object, "titleId", entry.title_id);
        read_string(object, "title", entry.title);
        read_string(object, "gameId", entry.game_id);
        read_string(object, "region", entry.region);
        read_string(object, "version", entry.version);
        read_string(object, "publisher", entry.publisher);
        read_string(object, "releaseDate", entry.release_date);
        read_string(object, "cover", entry.cover);
        read_string(object, "hero", entry.hero);
        read_string(object, "contentType", entry.content_type);
        read_string(object, "url", entry.download_url);
        read_string(object, "filename", entry.filename);
        entry.size_bytes = read_number(object, "sizeBytes");
        // Unknown entries stay blocked. Only an explicit, curated homebrew marker
        // can make an entry eligible for the authorized-download path.
        std::string content_type;
        read_string(object, "contentType", content_type);
        entry.commercial = is_commercial_title_id(entry.title_id) ||
            !(read_boolean_false(object, "commercial") && content_type == "homebrew");
        if (!entry.id.empty() && !entry.title_id.empty() && !entry.title.empty())
            entries_.push_back(entry);
    }
    return true;
}
}
