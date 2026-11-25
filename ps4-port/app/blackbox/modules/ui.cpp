#include "ui.hpp"
#include "downloader.hpp"
#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/backends/imgui_impl_sdl2.h"
#include "../../third_party/imgui/backends/imgui_impl_sdlrenderer2.h"
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>
#include <orbis/Pad.h>

namespace blackbox {
namespace {
ImVec4 accent(int theme) {
    if (theme == 1) return ImVec4(0.31f, 0.88f, 0.65f, 1.0f);
    if (theme == 2) return ImVec4(0.72f, 0.48f, 1.0f, 1.0f);
    return ImVec4(0.0f, 0.70f, 1.0f, 1.0f);
}
std::string safe_filename(std::string value) {
    for (char& ch : value)
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '\n' || ch == '\r') ch = '_';
    return value;
}
std::string clipped_title(const std::string& value, float max_width) {
    if (ImGui::CalcTextSize(value.c_str()).x <= max_width) return value;
    std::string clipped = value;
    const std::string suffix = "...";
    while (!clipped.empty() && ImGui::CalcTextSize((clipped + suffix).c_str()).x > max_width) {
        // Remove one complete UTF-8 code point, not just its final byte.
        clipped.pop_back();
        while (!clipped.empty() && (static_cast<unsigned char>(clipped.back()) & 0xC0) == 0x80)
            clipped.pop_back();
    }
    return clipped.empty() ? suffix : clipped + suffix;
}
}

Ui::Ui(SDL_Window* window, SDL_Renderer* renderer, Storage& storage)
    : window_(window), renderer_(renderer), storage_(storage) {
    char* base = SDL_GetBasePath();
    data_path_ = base ? base : "/app0/";
    if (base) SDL_free(base);
    data_path_ += "data/";

    // Start directly in the catalogue; the home menu is intentionally bypassed.
    page_ = Page::Catalog;
    ensure_catalog_loaded();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // Controller navigation is handled by the explicit BLACKBOX selection model.
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.ChildRounding = 10.0f;
    style.FrameRounding = 7.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.WindowPadding = ImVec2(22, 18);
    style.FramePadding = ImVec2(14, 10);
    style.ItemSpacing = ImVec2(14, 12);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.025f, 0.045f, 0.075f, 0.97f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.045f, 0.075f, 0.115f, 0.94f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.10f, 0.22f, 0.32f, 0.9f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.07f, 0.15f, 0.23f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.04f, 0.34f, 0.49f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = accent(storage_.accent_theme());
    style.Colors[ImGuiCol_Header] = ImVec4(0.04f, 0.26f, 0.38f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.02f, 0.38f, 0.55f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = accent(storage_.accent_theme());
    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer2_Init(renderer_);
}
Ui::~Ui() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    for (auto& item : cover_cache_) if (item.second) SDL_DestroyTexture(item.second);
}
void Ui::process_event(const SDL_Event& event) { ImGui_ImplSDL2_ProcessEvent(&event); }
void Ui::set_controller_debug(bool connected, int32_t error, uint32_t buttons, uint32_t pressed, int navigation) {
    controller_connected_ = connected;
    controller_error_ = error;
    controller_buttons_ = buttons;
    controller_pressed_ = pressed;
    controller_navigation_ = navigation;
}
SDL_Texture* Ui::load_cover(const CatalogEntry& entry) {
    const auto found = cover_cache_.find(entry.title_id);
    if (found != cover_cache_.end()) return found->second;
    std::string safe;
    for (unsigned char ch : entry.title_id)
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || ch == '_' || ch == '-') safe.push_back((char)ch);
    if (safe.empty()) return nullptr;
    // Try SDL's executable-relative path first, then the PS4 app mount.
    SDL_Surface* surface = SDL_LoadBMP((data_path_ + "covers/" + safe + ".bmp").c_str());
    if (!surface) surface = SDL_LoadBMP(("/app0/data/covers/" + safe + ".bmp").c_str());
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_FreeSurface(surface);
    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        cover_cache_[entry.title_id] = texture;
    }
    return texture;
}
std::vector<std::size_t> Ui::favorite_indices() const {
    std::vector<std::size_t> result;
    const auto& entries = catalog_.entries();
    for (std::size_t i = 0; i < entries.size(); ++i)
        if (storage_.is_favorite(entries[i].id)) result.push_back(i);
    return result;
}
void Ui::ensure_catalog_loaded() {
    if (catalog_loaded_) return;
    std::string path = data_path_ + "catalog.json";
    if (!catalog_.load(path)) catalog_.load("/app0/data/catalog.json");
    catalog_loaded_ = true;
}
void Ui::handle_controller_buttons(uint32_t buttons) {
    // Keep the original PS MTP Explorer input contract: the controller layer
    // forwards the pressed-button bitmask; the UI decides what each button does.
    if (buttons & ORBIS_PAD_BUTTON_UP) move_selection(-6);
    else if (buttons & ORBIS_PAD_BUTTON_DOWN) move_selection(6);
    else if (buttons & ORBIS_PAD_BUTTON_LEFT) move_selection(-1);
    else if (buttons & ORBIS_PAD_BUTTON_RIGHT) move_selection(1);
    else if (buttons & ORBIS_PAD_BUTTON_L1) move_selection(-18);
    else if (buttons & ORBIS_PAD_BUTTON_R1) move_selection(18);
}
void Ui::move_selection(int delta) {
    if (delta == 0) return;
    const int direction = delta < 0 ? -1 : 1;
    if (page_ == Page::Home) {
        selected_ = std::max(0, std::min(2, selected_ + direction));
        return;
    }
    if (page_ == Page::Settings) {
        selected_setting_ = std::max(0, std::min(1, selected_setting_ + direction));
        return;
    }
    if (page_ == Page::Details) {
        selected_detail_action_ = delta < 0 ? 0 : 1;
        return;
    }
    if (page_ == Page::Favorites) {
        const auto ids = favorite_indices();
        if (ids.empty()) { selected_favorite_ = 0; return; }
        const int last = static_cast<int>(ids.size()) - 1;
        selected_favorite_ = static_cast<std::size_t>(std::max(0, std::min(last, static_cast<int>(selected_favorite_) + direction)));
        selected_item_ = ids[selected_favorite_];
        return;
    }
    if (page_ == Page::Catalog) {
        const auto& entries = catalog_.entries();
        std::vector<std::size_t> visible;
        visible.reserve(entries.size());
        for (std::size_t i = 0; i < entries.size(); ++i)
            if (search_query_[0] == '\0' || entries[i].title.find(search_query_) != std::string::npos ||
                entries[i].title_id.find(search_query_) != std::string::npos) visible.push_back(i);
        if (visible.empty()) return;
        auto it = std::find(visible.begin(), visible.end(), selected_item_);
        int pos = it == visible.end() ? 0 : static_cast<int>(it - visible.begin());
        // D-pad up/down = one row (six covers); left/right = one cover; L1/R1 = one page.
        pos = std::max(0, std::min(static_cast<int>(visible.size()) - 1, pos + delta));
        selected_item_ = visible[static_cast<std::size_t>(pos)];
    }
}
void Ui::activate() {
    if (page_ == Page::Home) {
        if (selected_ == 0) { ensure_catalog_loaded(); selected_item_ = 0; page_ = Page::Catalog; }
        else if (selected_ == 1) { ensure_catalog_loaded(); selected_favorite_ = 0; page_ = Page::Favorites; }
        else page_ = Page::Settings;
    } else if (page_ == Page::Catalog && !catalog_.entries().empty()) {
        details_origin_ = Page::Catalog; selected_detail_action_ = 0; page_ = Page::Details;
    } else if (page_ == Page::Favorites) {
        const auto ids = favorite_indices();
        if (selected_favorite_ < ids.size()) {
            selected_item_ = ids[selected_favorite_]; details_origin_ = Page::Favorites;
            selected_detail_action_ = 0; page_ = Page::Details;
        }
    } else if (page_ == Page::Details) {
        const auto& entries = catalog_.entries();
        if (selected_item_ < entries.size()) {
            if (selected_detail_action_ == 0) start_download(entries[selected_item_]);
            else toggle_favorite();
        }
    } else if (page_ == Page::Settings) {
        if (selected_setting_ == 0) storage_.set_accent_theme((storage_.accent_theme() + 1) % 3);
        else storage_.set_show_title_ids(!storage_.show_title_ids());
        storage_.save();
    }
}
bool Ui::toggle_favorite() {
    ensure_catalog_loaded();
    if (page_ != Page::Catalog && page_ != Page::Details && page_ != Page::Favorites) return false;
    if (page_ == Page::Favorites) {
        const auto ids = favorite_indices();
        if (selected_favorite_ >= ids.size()) return false;
        selected_item_ = ids[selected_favorite_];
    }
    if (selected_item_ >= catalog_.entries().size()) return false;
    const bool changed = storage_.toggle_favorite(catalog_.entries()[selected_item_].id);
    if (changed) {
        const auto ids = favorite_indices();
        if (ids.empty()) selected_favorite_ = 0;
        else {
            selected_favorite_ = std::min(selected_favorite_, ids.size() - 1);
            if (page_ == Page::Favorites) selected_item_ = ids[selected_favorite_];
        }
        storage_.save();
    }
    return changed;
}
bool Ui::back() {
    if (page_ == Page::Details) { page_ = details_origin_; selected_detail_action_ = 0; return true; }
    if (page_ == Page::Catalog) return true;
    if (page_ != Page::Home) { page_ = Page::Catalog; return true; }
    return false;
}
void Ui::start_download(const CatalogEntry& entry) {


    if (entry.download_url.empty() || entry.filename.empty()) {
        download_message_ = "Source homebrew indisponible.";
        return;
    }
    char* pref = SDL_GetPrefPath("Ridagames", "BlackBoxPS4");
    if (!pref) { download_message_ = "Impossible de créer le dossier de téléchargement."; return; }
    std::string directory = std::string(pref) + "downloads/";
    SDL_free(pref);
    mkdir(directory.c_str(), 0777);
    DownloadEntry request;
    request.url = entry.download_url;
    request.filename = safe_filename(entry.filename);
    request.output_path = directory + request.filename;
    request.commercial = entry.commercial;
    request.homebrew = entry.content_type == "homebrew";
    download_message_ = "Téléchargement homebrew en cours...";
    const bool ok = download_pkg(request, [this](const DownloadStatus& status) {
        if (status.total) download_message_ = "Téléchargement : " + std::to_string((status.downloaded*100ULL)/status.total) + "%";
        else if (!status.error.empty()) download_message_ = status.error;
    });
    download_message_ = ok ? "Téléchargement terminé : " + request.filename : "Échec du téléchargement : source ou réseau.";
}
void Ui::render_home() {
    ImGui::SetNextWindowPos(ImVec2(0,0)); ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("BLACKBOX HOME", nullptr, ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
    ImGui::TextColored(accent(storage_.accent_theme()), "BLACKBOX"); ImGui::SameLine(); ImGui::TextDisabled("PLAYSTATION 4  /  RIDAGAMES");
    ImGui::Separator(); ImGui::Dummy(ImVec2(0,70));
    ImGui::SetWindowFontScale(1.7f); ImGui::Text("YOUR LIBRARY. YOUR CONSOLE."); ImGui::SetWindowFontScale(1.0f);
    ImGui::TextDisabled("Browse the PS4 catalogue, manage favourites and settings.");
    ImGui::Dummy(ImVec2(0,45));
    const char* labels[] = {"CATALOGUE", "FAVORITES", "SETTINGS"};
    const char* desc[] = {"Browse PS4 titles and cover artwork", "Your saved collection", "Appearance and preferences"};
    for (int i=0;i<3;i++) {
        ImGui::PushID(i);
        ImGui::BeginChild("home-card", ImVec2(0,150), true);
        if (i==selected_) ImGui::TextColored(accent(storage_.accent_theme()), "● SELECTED");
        ImGui::SetWindowFontScale(1.35f); ImGui::Text("%s",labels[i]); ImGui::SetWindowFontScale(1.0f);
        ImGui::TextDisabled("%s",desc[i]);
        if (ImGui::Button("OPEN",ImVec2(130,38))) { selected_=i; activate(); }
        ImGui::EndChild(); ImGui::PopID();
    }
    ImGui::SetCursorPosY(ImGui::GetWindowHeight()-55); ImGui::Separator(); ImGui::TextDisabled("CROSS  OPEN     DPAD  NAVIGATE     CIRCLE  BACK");
    ImGui::End();
}
void Ui::render_catalog() {
    ImGui::SetNextWindowPos(ImVec2(0,0)); ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("PS4 CATALOGUE",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
    const auto& entries=catalog_.entries();
    std::vector<std::size_t> visible;
    visible.reserve(entries.size());
    for (std::size_t i=0;i<entries.size();++i)
        if (search_query_[0]=='\0' || entries[i].title.find(search_query_)!=std::string::npos ||
            entries[i].title_id.find(search_query_)!=std::string::npos) visible.push_back(i);
    ImGui::TextColored(accent(storage_.accent_theme()),"BLACKBOX"); ImGui::SameLine(); ImGui::Text(" / PS4 CATALOGUE");
    ImGui::SameLine(ImGui::GetWindowWidth()-280); ImGui::TextDisabled("%u / %u TITLES",(unsigned)visible.size(),(unsigned)entries.size());
    ImGui::Separator();
    ImGui::SetNextItemWidth(520.0f);
    ImGui::InputTextWithHint("##catalog-search","Search a game or title ID...",search_query_,sizeof(search_query_));
    ImGui::SameLine(); if(ImGui::Button("CLEAR",ImVec2(90,34))) search_query_[0]='\0';
    // Put the input diagnostic INSIDE the catalogue window so it cannot be
    // hidden behind another ImGui window or lost due to window ordering.
    ImGui::Separator();
    ImGui::TextColored(controller_connected_ ? ImVec4(0.20f,1.0f,0.35f,1.0f) : ImVec4(1.0f,0.35f,0.25f,1.0f),
        "PAD: %s   ERROR: 0x%08X", controller_connected_ ? "CONNECTED" : "WAITING",
        static_cast<unsigned int>(controller_error_));
    ImGui::SameLine();
    ImGui::Text("RAW: 0x%08X   PRESSED: 0x%08X   NAV: %d   SELECTED: %u",
        static_cast<unsigned int>(controller_buttons_),
        static_cast<unsigned int>(controller_pressed_), controller_navigation_,
        static_cast<unsigned int>(selected_item_));
    visible.clear();
    for (std::size_t i=0;i<entries.size();++i)
        if (search_query_[0]=='\0' || entries[i].title.find(search_query_)!=std::string::npos ||
            entries[i].title_id.find(search_query_)!=std::string::npos) visible.push_back(i);
    if (!visible.empty() && std::find(visible.begin(),visible.end(),selected_item_)==visible.end())
        selected_item_=visible.front();
    const std::size_t page_size=18;
    std::size_t selected_pos=0;
    auto it=std::find(visible.begin(),visible.end(),selected_item_);
    if(it!=visible.end()) selected_pos=(std::size_t)(it-visible.begin());
    const std::size_t page=selected_pos/page_size, first=page*page_size;
    ImGui::BeginChild("cover-grid",ImVec2(0,-92),false);
    const float gap=16.0f, width=ImGui::GetContentRegionAvail().x;
    const int columns=6; const float cardW=(width-gap*(columns-1))/columns;
    const std::size_t last=std::min(visible.size(),first+page_size);
    for(std::size_t pos=first;pos<last;++pos){
        const std::size_t i=visible[pos]; const auto& e=entries[i]; ImGui::PushID((int)i);
        if((int)((pos-first)%columns)!=0) ImGui::SameLine(0,gap);
        ImGui::BeginGroup(); SDL_Texture* tex=load_cover(e); const bool selected=(i==selected_item_);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,selected?3.0f:1.0f);
        if(selected) ImGui::PushStyleColor(ImGuiCol_Border,accent(storage_.accent_theme()));
        bool clicked=false;
        if(tex) clicked=ImGui::ImageButton("cover",(ImTextureID)(intptr_t)tex,ImVec2(cardW,cardW*1.12f));
        else clicked=ImGui::Button("NO COVER",ImVec2(cardW,cardW*1.12f));
        if(selected) ImGui::PopStyleColor(); ImGui::PopStyleVar();
        if(clicked){selected_item_=i;details_origin_=Page::Catalog;selected_detail_action_=0;page_=Page::Details;}
        const std::string short_title=clipped_title(e.title,cardW);
        if(selected) ImGui::TextColored(accent(storage_.accent_theme()),"%s",short_title.c_str());
        else ImGui::Text("%s",short_title.c_str());
        if(storage_.show_title_ids()) ImGui::TextDisabled("%s",e.title_id.c_str());
        ImGui::EndGroup(); ImGui::PopID();
    }
    if(visible.empty()) ImGui::TextDisabled("No title matches your search.");
    ImGui::EndChild(); ImGui::Separator();
    if(ImGui::Button("PREVIOUS",ImVec2(130,38)) && page>0) selected_item_=visible[(page-1)*page_size];
    ImGui::SameLine(); ImGui::TextDisabled("PAGE %u / %u",(unsigned)(page+1),(unsigned)std::max<std::size_t>(1,(visible.size()+page_size-1)/page_size));
    ImGui::SameLine(); if(ImGui::Button("NEXT",ImVec2(130,38)) && first+page_size<visible.size()) selected_item_=visible[first+page_size];
    ImGui::SameLine(); ImGui::TextDisabled("DPAD NAVIGATE   CROSS DETAILS   SQUARE FAVORITE   CIRCLE BACK");
    ImGui::End();
}
void Ui::render_favorites() {
    ImGui::SetNextWindowPos(ImVec2(0,0)); ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("FAVORITES",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
    auto ids=favorite_indices(); ImGui::TextColored(accent(storage_.accent_theme()),"FAVORITES"); ImGui::SameLine(); ImGui::TextDisabled("%u SAVED",(unsigned)ids.size()); ImGui::Separator();
    ImGui::BeginChild("favorite-list",ImVec2(0,-50),true);
    for (std::size_t n=0;n<ids.size();++n) {
        const auto& e=catalog_.entries()[ids[n]];
        if (ImGui::Selectable(e.title.c_str(),n==selected_favorite_,0,ImVec2(0,58))) { selected_favorite_=n; selected_item_=ids[n]; details_origin_=Page::Favorites; selected_detail_action_=0; page_=Page::Details; }
        ImGui::SameLine(); ImGui::TextDisabled("%s",e.title_id.c_str());
    }
    if (ids.empty()) ImGui::TextDisabled("No favorites yet. Save titles from the catalogue.");
    ImGui::EndChild(); ImGui::TextDisabled("CROSS  DETAILS     SQUARE  REMOVE     CIRCLE  BACK"); ImGui::End();
}
void Ui::render_details() {
    if (selected_item_ >= catalog_.entries().size()) return;
    const auto& e = catalog_.entries()[selected_item_];
    ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("TITLE DETAILS", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    ImGui::TextColored(accent(storage_.accent_theme()), "BLACKBOX"); ImGui::SameLine(); ImGui::Text("/ TITLE DETAILS"); ImGui::Separator();
    ImGui::BeginChild("details-cover", ImVec2(370, 0), true);
    SDL_Texture* tex = load_cover(e);
    if (tex) ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(320, 370));
    else ImGui::TextDisabled("COVER NOT AVAILABLE");
    ImGui::EndChild(); ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::SetWindowFontScale(1.45f); ImGui::TextWrapped("%s", e.title.c_str()); ImGui::SetWindowFontScale(1.0f);
    ImGui::TextDisabled("TITLE ID  %s", e.title_id.c_str()); ImGui::Separator();
    ImGui::Text("Region: %s", e.region.c_str()); ImGui::Text("Version: %s", e.version.c_str());
    ImGui::Text("Publisher: %s", e.publisher.c_str()); ImGui::Text("Release: %s", e.release_date.c_str());
    ImGui::Text("Game ID: %s", e.game_id.c_str()); ImGui::Spacing();
    const bool download_selected = selected_detail_action_ == 0;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, download_selected ? 3.0f : 1.0f);
    if (download_selected) ImGui::PushStyleColor(ImGuiCol_Border, accent(storage_.accent_theme()));
    if (ImGui::Button("DOWNLOAD PKG", ImVec2(220, 54))) start_download(e);
    if (download_selected) ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::SameLine();
    const bool favorite_selected = selected_detail_action_ == 1;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, favorite_selected ? 3.0f : 1.0f);
    if (favorite_selected) ImGui::PushStyleColor(ImGuiCol_Border, accent(storage_.accent_theme()));
    if (ImGui::Button(storage_.is_favorite(e.id) ? "REMOVE FAVORITE" : "ADD FAVORITE", ImVec2(220, 54))) toggle_favorite();
    if (favorite_selected) ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    if (e.commercial || e.content_type != "homebrew") ImGui::TextColored(ImVec4(1, .35f, .3f, 1), "COMMERCIAL CONTENT — DOWNLOAD BLOCKED");
    if (!download_message_.empty()) ImGui::TextWrapped("%s", download_message_.c_str());
    ImGui::EndGroup(); ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 45); ImGui::Separator();
    ImGui::TextDisabled("DPAD  SELECT ACTION   CROSS  CONFIRM   SQUARE  FAVORITE   CIRCLE  BACK");
    ImGui::End();
}
void Ui::render_settings() {
    ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("SETTINGS", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    ImGui::TextColored(accent(storage_.accent_theme()), "SETTINGS"); ImGui::Separator();
    const bool theme_selected = selected_setting_ == 0;
    ImGui::TextColored(theme_selected ? accent(storage_.accent_theme()) : ImVec4(1,1,1,1), "Interface accent");
    ImGui::SameLine(360);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, theme_selected ? 3.0f : 1.0f);
    if (theme_selected) ImGui::PushStyleColor(ImGuiCol_Border, accent(storage_.accent_theme()));
    if (ImGui::Button("CHANGE THEME")) { selected_setting_ = 0; activate(); }
    if (theme_selected) ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    const bool ids_selected = selected_setting_ == 1;
    ImGui::TextColored(ids_selected ? accent(storage_.accent_theme()) : ImVec4(1,1,1,1), "Show title IDs");
    ImGui::SameLine(360);
    bool show = storage_.show_title_ids();
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, ids_selected ? 3.0f : 1.0f);
    if (ids_selected) ImGui::PushStyleColor(ImGuiCol_Border, accent(storage_.accent_theme()));
    if (ImGui::Checkbox("##showids", &show)) { selected_setting_ = 1; storage_.set_show_title_ids(show); storage_.save(); }
    if (ids_selected) ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::TextDisabled("DPAD  SELECT   CROSS  CHANGE   CIRCLE  BACK");
    ImGui::End();
}
void Ui::render() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    switch (page_) {
        case Page::Home: render_home(); break;
        case Page::Catalog: render_catalog(); break;
        case Page::Details: render_details(); break;
        case Page::Favorites: render_favorites(); break;
        case Page::Settings: render_settings(); break;
    }
    // High-contrast diagnostic kept on-screen while testing controller/UI routing.
    ImGui::SetNextWindowPos(ImVec2(18, 18), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.96f);
    ImGui::Begin("BLACKBOX INPUT TEST", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::TextColored(controller_connected_ ? ImVec4(0.2f,1.0f,0.35f,1.0f) : ImVec4(1.0f,0.35f,0.25f,1.0f), "PAD: %s", controller_connected_ ? "CONNECTED" : "WAITING");
    ImGui::Text("ERROR: 0x%08X", static_cast<unsigned int>(controller_error_));
    ImGui::Text("RAW BUTTONS: 0x%08X", static_cast<unsigned int>(controller_buttons_));
    ImGui::Text("PRESSED: 0x%08X", static_cast<unsigned int>(controller_pressed_));
    ImGui::Text("NAV DELTA: %d", controller_navigation_);
    ImGui::Text("UI SELECTION: %u", static_cast<unsigned int>(selected_item_));
    ImGui::End();
    ImGui::Render();
    SDL_SetRenderDrawColor(renderer_, 5, 9, 17, 255); SDL_RenderClear(renderer_);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer_);
}
}
