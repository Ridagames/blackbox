#pragma once
#include <string>
#include <vector>

namespace blackbox {
class Storage {
public:
    bool load();
    bool save() const;
    void set_last_section(int section) { last_section_ = section; }
    int last_section() const { return last_section_; }
    bool is_favorite(const std::string& id) const;
    bool toggle_favorite(const std::string& id);
    const std::vector<std::string>& favorites() const { return favorites_; }

private:
    int last_section_ = 0;
    std::string path_;
    std::vector<std::string> favorites_;
};
}
