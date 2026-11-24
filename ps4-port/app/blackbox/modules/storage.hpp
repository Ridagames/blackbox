#pragma once
#include <string>
namespace blackbox {
class Storage {
public:
    bool load();
    bool save() const;
    void set_last_section(int section) { last_section_ = section; }
    int last_section() const { return last_section_; }
private:
    int last_section_ = 0;
    std::string path_;
};
}
