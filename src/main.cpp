#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <iostream>
#include <vector>
#include <string>

using namespace ftxui;

int main() {
    auto screen = ScreenInteractive::Fullscreen();

    std::vector<std::string> books = {"Genesis", "Exodus", "Leviticus", "Numbers", "Deuteronomy", "Psalms", "Proverbs"};
    int selected_book = 0;
    auto book_selector = Menu(&books, &selected_book);

    auto verse_content = Renderer([] {
        return window(text("PSALM 23:1"),
            vbox({
                text("The LORD is my shepherd; I shall not want."),
                separator(),
                text("He maketh me to lie down in green pastures:"),
                text("he leadeth me beside the still waters."),
            })
        ) | color(Color::Cyan);
    });

    auto layout = Container::Horizontal({
        book_selector,
        verse_content
    });

    auto component = Renderer(layout, [&] {
        return hbox({
            book_selector->Render() | size(WIDTH, EQUAL, 25) | border,
            verse_content->Render() | flex | border
        }) | borderDouble | color(Color::Magenta);
    });

    screen.Loop(component);

    return 0;
}
