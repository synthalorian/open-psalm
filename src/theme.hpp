#pragma once
#include <string>
#include <vector>
#include <ftxui/dom/elements.hpp>
#include "types.hpp"

namespace ThemeManager {
    std::vector<Theme> getAllThemes();
    Theme getTheme(const std::string& name);
    ftxui::Element apply(const Theme& t, ftxui::Element element);
    ftxui::Element styled(const Theme& t, ftxui::Element element, const std::string& style);
    ftxui::Element text(const Theme& t, const std::string& str, const std::string& style);
}
