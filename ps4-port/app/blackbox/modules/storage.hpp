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

    int accent_theme() const { return accent_theme_; }
    void set_accent_theme(int theme) { accent_theme_ = (theme % 3 + 3) % 3; }
    bool show_title_ids() const { return show_title_ids_; }
    void set_show_title_ids(bool value) { show_title_ids_ = value; }

private:
    int last_section_ = 0;
    int accent_theme_ = 0;
    bool show_title_ids_ = true;
    std::string path_;
    std::vector<std::string> favorites_;
};
}
