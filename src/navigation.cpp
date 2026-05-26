#include "app.hpp"
#include <algorithm>

// ============================================================
// Navigation — book/chapter/content scrolling, history
// ============================================================

void App::navigateBook(int delta) {
    int new_idx = selected_book_ + delta;
    if (new_idx >= 0 && new_idx < (int)books_.size()) {
        pushNavHistory();
        selected_book_ = new_idx;
        chapters_ = db_->getChapters(books_[selected_book_]);
        selected_chapter_ = chapters_.empty() ? 1 : chapters_[0];
        content_scroll_ = 0;
        loadChapter();
        showNotification(books_[selected_book_]);
    }
}

void App::navigateChapter(int delta) {
    chapters_ = db_->getChapters(books_[selected_book_]);
    if (chapters_.empty()) return;

    auto it = std::lower_bound(chapters_.begin(), chapters_.end(), selected_chapter_);
    int idx = std::distance(chapters_.begin(), it);
    int new_idx = idx + delta;
    if (new_idx >= 0 && new_idx < (int)chapters_.size()) {
        selected_chapter_ = chapters_[new_idx];
        content_scroll_ = 0;
        loadChapter();
        showNotification(books_[selected_book_] + " " + std::to_string(selected_chapter_));
    }
}

void App::navigateContent(int delta) {
    int max_scroll = std::max(0, (int)current_verses_.size() - 1);
    content_scroll_ = std::clamp(content_scroll_ + delta, 0, max_scroll);
    notification_timer_ = 0;
}

void App::navigateSearchResult(int delta) {
    if (search_results_.empty()) return;
    int max_idx = search_results_.size() - 1;
    search_result_index_ = std::clamp(search_result_index_ + delta, 0, max_idx);
    showNotification("Result " + std::to_string(search_result_index_ + 1) + "/" +
                     std::to_string(search_results_.size()));
}

void App::pushNavHistory() {
    if (books_.empty()) return;
    NavPosition pos;
    pos.book = books_[selected_book_];
    pos.chapter = selected_chapter_;
    pos.verse = content_scroll_ >= 0 && content_scroll_ < (int)current_verses_.size()
                ? current_verses_[content_scroll_].verse : 1;
    nav_back_.push_back(pos);
    nav_forward_.clear();
}

void App::goBack() {
    if (nav_back_.empty()) {
        showNotification("No previous position");
        return;
    }

    if (!books_.empty()) {
        NavPosition cur;
        cur.book = books_[selected_book_];
        cur.chapter = selected_chapter_;
        cur.verse = content_scroll_ >= 0 && content_scroll_ < (int)current_verses_.size()
                    ? current_verses_[content_scroll_].verse : 1;
        nav_forward_.push_back(cur);
    }

    NavPosition prev = nav_back_.back();
    nav_back_.pop_back();

    jumpToLocation(prev.book, prev.chapter, prev.verse);
    showNotification("Back to " + prev.book + " " + std::to_string(prev.chapter));
}

void App::goForward() {
    if (nav_forward_.empty()) {
        showNotification("No forward position");
        return;
    }

    if (!books_.empty()) {
        NavPosition cur;
        cur.book = books_[selected_book_];
        cur.chapter = selected_chapter_;
        cur.verse = content_scroll_ >= 0 && content_scroll_ < (int)current_verses_.size()
                    ? current_verses_[content_scroll_].verse : 1;
        nav_back_.push_back(cur);
    }

    NavPosition next = nav_forward_.back();
    nav_forward_.pop_back();

    jumpToLocation(next.book, next.chapter, next.verse);
    showNotification("Forward to " + next.book + " " + std::to_string(next.chapter));
}

void App::jumpToLocation(const std::string& book, int chapter, int verse) {
    for (size_t i = 0; i < books_.size(); i++) {
        if (books_[i] == book) {
            selected_book_ = i;
            selected_chapter_ = chapter;
            loadChapter();
            content_scroll_ = 0;
            for (size_t vi = 0; vi < current_verses_.size(); vi++) {
                if (current_verses_[vi].verse >= verse) {
                    content_scroll_ = vi;
                    break;
                }
            }
            focus_panel_ = 1;
            break;
        }
    }
}
