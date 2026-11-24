#include "blackbox/catalog_filter.hpp"
#include "blackbox/catalog_repository.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
namespace {
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
}
int main() {
    const std::string sample = R"JSON({
      "schemaVersion": 1, "revision": 1,
      "releases": [
        {"id":"ps4-001","titleId":"CUSA00001","title":"Sample PS4 Title",
         "platform":"ps4","region":"EUR","version":"01.00","sizeBytes":1024,
         "cover":"https://example.invalid/cover.webp","hero":"",
         "url":"https://example.invalid/package.pkg"},
        {"id":"ps5-001","titleId":"PPSA00001","title":"PS5 Must Not Appear","platform":"ps5"}
      ]
    })JSON";
    const auto parsed = blackbox::parse_catalog(sample);
    check(parsed.ok, "valid catalogue should parse");
    check(parsed.entries.size() == 1, "only PS4 metadata should be retained");
    check(parsed.entries[0].title == "Sample PS4 Title", "title should be parsed");
    check(parsed.entries[0].size_bytes == 1024, "size should be parsed");
    check(blackbox::filter_catalog(parsed.entries, "sample").size() == 1, "title search");
    check(blackbox::filter_catalog(parsed.entries, "", "EUR").size() == 1, "region filter");
    check(blackbox::filter_catalog(parsed.entries, "missing").empty(), "empty search result");
    check(!blackbox::parse_catalog("{not-json").ok, "invalid JSON should fail");
    check(!blackbox::parse_catalog(R"({"schemaVersion":1})").ok, "missing releases should fail");
    std::cout << "All BlackBox PS4 core tests passed.\n";
    return 0;
}
