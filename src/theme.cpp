#include "theme.hpp"

namespace ThemeManager {

std::vector<Theme> getAllThemes() {
    return {
        {
            .name = "Blackshield",
            .background = ftxui::Color::RGB(13, 13, 17),          // #0D0D11 void
            .foreground = ftxui::Color::RGB(216, 211, 200),       // #D8D3C8 bone
            .accent = ftxui::Color::RGB(193, 18, 31),             // #C1121F blood
            .highlight = ftxui::Color::RGB(201, 162, 39),         // #C9A227 war-gold
            .secondary = ftxui::Color::RGB(123, 157, 196),        // #7B9DC4 steel-blue bright
            .border = ftxui::Color::RGB(138, 143, 152),           // #8A8F98 ash
            .title = ftxui::Color::RGB(245, 241, 232),            // #F5F1E8 bone-bright
        },
        {
            .name = "Synthwave '84",
            .background = ftxui::Color::RGB(36, 0, 55),         // #240037 deep dark purple (omarchy)
            .foreground = ftxui::Color::RGB(255, 255, 255),     // Pure white text
            .accent = ftxui::Color::RGB(143, 0, 255),           // #8f00ff vibrant purple
            .highlight = ftxui::Color::RGB(243, 231, 15),       // #f3e70f neon yellow
            .secondary = ftxui::Color::RGB(255, 126, 219),      // #ff7edb hot pink
            .border = ftxui::Color::RGB(143, 0, 255),           // Purple border
            .title = ftxui::Color::RGB(255, 0, 255),            // #ff00ff magenta
        },
        {
            .name = "Sunset",
            .background = ftxui::Color::RGB(30, 15, 25),
            .foreground = ftxui::Color::RGB(230, 200, 180),
            .accent = ftxui::Color::RGB(255, 100, 50),
            .highlight = ftxui::Color::RGB(255, 200, 50),
            .secondary = ftxui::Color::RGB(200, 80, 120),
            .border = ftxui::Color::RGB(180, 80, 40),
            .title = ftxui::Color::RGB(255, 150, 80),
        },
        {
            .name = "Chrome",
            .background = ftxui::Color::RGB(18, 22, 28),
            .foreground = ftxui::Color::RGB(200, 210, 220),
            .accent = ftxui::Color::RGB(0, 200, 230),
            .highlight = ftxui::Color::RGB(0, 255, 200),
            .secondary = ftxui::Color::RGB(100, 150, 200),
            .border = ftxui::Color::RGB(60, 80, 120),
            .title = ftxui::Color::RGB(0, 220, 255),
        },
        {
            .name = "Matrix",
            .background = ftxui::Color::RGB(0, 5, 0),
            .foreground = ftxui::Color::RGB(0, 200, 0),
            .accent = ftxui::Color::RGB(0, 255, 65),
            .highlight = ftxui::Color::RGB(150, 255, 150),
            .secondary = ftxui::Color::RGB(0, 150, 0),
            .border = ftxui::Color::RGB(0, 100, 0),
            .title = ftxui::Color::RGB(0, 255, 100),
        },
        {
            .name = "Dracula",
            .background = ftxui::Color::RGB(40, 42, 54),
            .foreground = ftxui::Color::RGB(248, 248, 242),
            .accent = ftxui::Color::RGB(189, 147, 249),       // Purple
            .highlight = ftxui::Color::RGB(80, 250, 123),      // Green
            .secondary = ftxui::Color::RGB(255, 121, 198),     // Pink
            .border = ftxui::Color::RGB(98, 114, 164),
            .title = ftxui::Color::RGB(255, 184, 108),        // Orange
        },
        {
            .name = "Nord",
            .background = ftxui::Color::RGB(46, 52, 64),
            .foreground = ftxui::Color::RGB(216, 222, 233),
            .accent = ftxui::Color::RGB(94, 129, 172),        // Frost blue
            .highlight = ftxui::Color::RGB(136, 192, 208),     // Light teal
            .secondary = ftxui::Color::RGB(180, 190, 210),
            .border = ftxui::Color::RGB(76, 86, 106),
            .title = ftxui::Color::RGB(143, 188, 187),        // Aurora green
        },
        {
            .name = "Gruvbox",
            .background = ftxui::Color::RGB(40, 40, 40),
            .foreground = ftxui::Color::RGB(235, 219, 178),
            .accent = ftxui::Color::RGB(211, 134, 155),       // Purple
            .highlight = ftxui::Color::RGB(184, 187, 38),      // Yellow-green
            .secondary = ftxui::Color::RGB(214, 93, 14),       // Orange
            .border = ftxui::Color::RGB(102, 92, 84),
            .title = ftxui::Color::RGB(250, 189, 47),         // Yellow
        },
        {
            .name = "Catppuccin Mocha",
            .background = ftxui::Color::RGB(30, 30, 46),
            .foreground = ftxui::Color::RGB(205, 214, 244),
            .accent = ftxui::Color::RGB(137, 180, 250),       // Blue
            .highlight = ftxui::Color::RGB(166, 227, 161),     // Green
            .secondary = ftxui::Color::RGB(245, 194, 231),     // Pink
            .border = ftxui::Color::RGB(69, 71, 90),
            .title = ftxui::Color::RGB(249, 226, 175),        // Yellow
        },
        {
            .name = "Tokyo Night",
            .background = ftxui::Color::RGB(26, 27, 38),
            .foreground = ftxui::Color::RGB(169, 192, 232),
            .accent = ftxui::Color::RGB(122, 162, 247),       // Blue
            .highlight = ftxui::Color::RGB(158, 206, 106),     // Green
            .secondary = ftxui::Color::RGB(187, 154, 247),     // Purple
            .border = ftxui::Color::RGB(65, 72, 104),
            .title = ftxui::Color::RGB(224, 175, 104),        // Orange
        },
        {
            .name = "Monokai",
            .background = ftxui::Color::RGB(39, 40, 34),
            .foreground = ftxui::Color::RGB(242, 242, 242),
            .accent = ftxui::Color::RGB(102, 217, 239),       // Cyan
            .highlight = ftxui::Color::RGB(166, 226, 46),      // Green
            .secondary = ftxui::Color::RGB(249, 38, 114),      // Pink/Red
            .border = ftxui::Color::RGB(89, 89, 89),
            .title = ftxui::Color::RGB(253, 151, 31),         // Orange
        },
        {
            .name = "Sepia (light)",
            .background = ftxui::Color::RGB(250, 235, 215),
            .foreground = ftxui::Color::RGB(60, 40, 30),
            .accent = ftxui::Color::RGB(139, 90, 43),         // Brown
            .highlight = ftxui::Color::RGB(180, 60, 20),       // Rust
            .secondary = ftxui::Color::RGB(160, 120, 80),      // Tan
            .border = ftxui::Color::RGB(200, 170, 130),
            .title = ftxui::Color::RGB(120, 60, 30),          // Dark brown
        },
    };
}

Theme getTheme(const std::string& name) {
    for (const auto& t : getAllThemes()) {
        if (t.name == name) return t;
    }
    return getAllThemes()[0];
}

ftxui::Element apply(const Theme& t, ftxui::Element element) {
    return element | ftxui::bgcolor(t.background) | ftxui::color(t.foreground);
}

ftxui::Element styled(const Theme& t, ftxui::Element element, const std::string& style) {
    if (style == "accent") return element | ftxui::color(t.accent);
    if (style == "highlight") return element | ftxui::color(t.highlight);
    if (style == "secondary") return element | ftxui::color(t.secondary);
    if (style == "border") return element | ftxui::color(t.border);
    if (style == "title") return element | ftxui::color(t.title);
    return element | ftxui::color(t.foreground);
}

ftxui::Element text(const Theme& t, const std::string& str, const std::string& style) {
    return styled(t, ftxui::text(str), style);
}

} // namespace ThemeManager
