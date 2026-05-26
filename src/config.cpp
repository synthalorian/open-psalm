#include "app.hpp"
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cctype>
#include <filesystem>
namespace fs = std::filesystem;

// ============================================================
// Config persistence — saveConfig / loadConfig
// ============================================================

void App::saveConfig() {
    config_.last_book = books_.empty() ? "" : books_[selected_book_];
    config_.last_chapter = selected_chapter_;
    config_.theme_index = theme_index_;
    config_.translation_code = db_->translationCode();
    config_.bookmarks = bookmarks_;
    config_.reading_plan_day = reading_plan_day_;
    config_.search_history = search_history_;
    config_.parallel_enabled = parallel_mode_;
    config_.parallel_second_translation = parallel_db_ ? parallel_db_->translationCode() : "WEB";

    const char* home_env = std::getenv("HOME");
    std::string home = home_env ? home_env : ".";
    std::string config_dir = home + "/.config/open-psalm";

    try {
        fs::create_directories(config_dir);
    } catch (...) {
        return;
    }

    std::ofstream file(config_dir + "/config.json");
    if (!file.is_open()) return;

    file << "{\n";
    file << "  \"last_book\": \"" << config_.last_book << "\",\n";
    file << "  \"last_chapter\": " << config_.last_chapter << ",\n";
    file << "  \"theme_index\": " << config_.theme_index << ",\n";
    file << "  \"translation_code\": \"" << config_.translation_code << "\",\n";
    file << "  \"reading_plan_day\": " << config_.reading_plan_day << ",\n";
    file << "  \"parallel_enabled\": " << (config_.parallel_enabled ? "true" : "false") << ",\n";
    file << "  \"parallel_second_translation\": \"" << config_.parallel_second_translation << "\",\n";
    file << "  \"search_history\": [\n";
    for (size_t i = 0; i < config_.search_history.size(); i++) {
        file << "    \"" << config_.search_history[i] << "\"";
        if (i < config_.search_history.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";
    file << "  \"bookmarks\": [\n";
    for (size_t i = 0; i < config_.bookmarks.size(); i++) {
        auto& bm = config_.bookmarks[i];                file << "    {\"book\": \"" << bm.book << "\", \"chapter\": " << bm.chapter
                     << ", \"verse\": " << bm.verse << ", \"label\": \"" << bm.label
                     << "\", \"note\": \"" << bm.note << "\"}";
        if (i < config_.bookmarks.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";
    file << "  \"votd_autocopy\": " << (config_.votd_autocopy ? "true" : "false") << ",\n";
    file << "  \"cross_references_enabled\": " << (config_.cross_references_enabled ? "true" : "false") << ",\n";
    file << "  \"last_export_dir\": \"" << config_.last_export_dir << "\"\n";
    file << "}\n";
    file.close();
}

void App::loadConfig() {
    const char* home_env = std::getenv("HOME");
    std::string home = home_env ? home_env : ".";
    std::string config_path = home + "/.config/open-psalm/config.json";

    std::ifstream file(config_path);
    if (!file.is_open()) return;

    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    file.close();

    auto safe_stoi = [](const std::string& s, int default_val = 0) -> int {
        try {
            if (s.empty()) return default_val;
            return std::stoi(s);
        } catch (...) {
            return default_val;
        }
    };

    // Only search in the portion before "bookmarks" for top-level keys
    std::string top_section = content;
    size_t bm_pos = content.find("\"bookmarks\"");
    if (bm_pos != std::string::npos) {
        top_section = content.substr(0, bm_pos);
    }

    auto find_top_key = [&](const std::string& key) -> std::string {
        size_t pos = top_section.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        pos = top_section.find(':', pos);
        if (pos == std::string::npos) return "";
        pos++;
        while (pos < top_section.size() && (top_section[pos] == ' ' || top_section[pos] == '\t' || top_section[pos] == '\n')) pos++;
        if (pos >= top_section.size()) return "";
        if (top_section[pos] == '"') {
            pos++;
            std::string val;
            while (pos < top_section.size() && top_section[pos] != '"') {
                val += top_section[pos++];
            }
            return val;
        }
        std::string val;
        while (pos < top_section.size() && isdigit((unsigned char)top_section[pos])) {
            val += top_section[pos++];
        }
        return val;
    };

    config_.last_book = find_top_key("last_book");
    config_.last_chapter = safe_stoi(find_top_key("last_chapter"), 1);
    config_.theme_index = safe_stoi(find_top_key("theme_index"), 0);
    config_.translation_code = find_top_key("translation_code");
    if (config_.translation_code.empty()) config_.translation_code = "KJV";
    config_.reading_plan_day = safe_stoi(find_top_key("reading_plan_day"), 0);
    config_.parallel_enabled = find_top_key("parallel_enabled") == "true";
    config_.parallel_second_translation = find_top_key("parallel_second_translation");
    if (config_.parallel_second_translation.empty()) config_.parallel_second_translation = "WEB";
    config_.votd_autocopy = find_top_key("votd_autocopy") == "true";
    config_.cross_references_enabled = find_top_key("cross_references_enabled") != "false";
    config_.last_export_dir = find_top_key("last_export_dir");
    if (config_.last_export_dir.empty()) config_.last_export_dir = "~/.config/open-psalm/exports";

    // Parse search history
    if (bm_pos != std::string::npos) {
        std::string search_section = content.substr(0, bm_pos);
        size_t arr_start = search_section.rfind('[');
        if (arr_start != std::string::npos) {
            size_t arr_end = search_section.rfind(']');
            if (arr_end != std::string::npos && arr_end > arr_start) {
                std::string arr = search_section.substr(arr_start, arr_end - arr_start + 1);
                size_t qpos = 0;
                while ((qpos = arr.find('"', qpos)) != std::string::npos) {
                    qpos++;
                    std::string q;
                    while (qpos < arr.size() && arr[qpos] != '"') {
                        q += arr[qpos++];
                    }
                    if (!q.empty()) {
                        config_.search_history.push_back(q);
                    }
                    if (qpos < arr.size()) qpos++;
                }
            }
        }
    }

    // Parse bookmarks from the bookmarks array
    if (bm_pos != std::string::npos) {
        size_t arr_start = content.find('[', bm_pos);
        if (arr_start != std::string::npos) {
            size_t arr_end = content.find(']', arr_start);
            if (arr_end != std::string::npos) {
                std::string arr = content.substr(arr_start, arr_end - arr_start + 1);
                size_t obj_pos = 0;
                while ((obj_pos = arr.find('{', obj_pos)) != std::string::npos) {
                    size_t obj_end = arr.find('}', obj_pos);
                    if (obj_end == std::string::npos) break;
                    std::string obj = arr.substr(obj_pos, obj_end - obj_pos + 1);

                    auto find_bk = [&](const std::string& key) -> std::string {
                        size_t p = obj.find("\"" + key + "\"");
                        if (p == std::string::npos) return "";
                        p = obj.find(':', p);
                        if (p == std::string::npos) return "";
                        p++;
                        while (p < obj.size() && (obj[p] == ' ' || obj[p] == '\t')) p++;
                        if (p >= obj.size()) return "";
                        if (obj[p] == '"') { p++; std::string v; while (p < obj.size() && obj[p] != '"') v += obj[p++]; return v; }
                        std::string v; while (p < obj.size() && isdigit((unsigned char)obj[p])) v += obj[p++]; return v;
                    };

                    Bookmark bm;
                    bm.book = find_bk("book");
                    std::string ch = find_bk("chapter");
                    std::string vs = find_bk("verse");
                    bm.label = find_bk("label");
                    bm.note = find_bk("note");
                    if (!bm.book.empty() && !ch.empty()) {
                        bm.chapter = safe_stoi(ch, 1);
                        bm.verse = vs.empty() ? 1 : safe_stoi(vs, 1);
                        config_.bookmarks.push_back(bm);
                    }

                    obj_pos = obj_end + 1;
                }
            }
        }
    }

    bookmarks_ = config_.bookmarks;
    reading_plan_day_ = config_.reading_plan_day;
    search_history_ = config_.search_history;
    if (!search_history_.empty()) {
        search_history_index_ = (int)search_history_.size();
    }
}
