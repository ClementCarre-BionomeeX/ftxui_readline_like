// line_editor.hpp
#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

inline auto CTRL(char c) {
  return ftxui::Event::Character(std::string(1, c & 0x1F));
}

class LineEditor : public ftxui::ComponentBase {
public:
  LineEditor() {}

  const std::string &GetLine() const { return buffer_; }
  void Clear() {
    buffer_.clear();
    cursor_pos_ = 0;
  }

  ftxui::Element Render() {
    using namespace ftxui;

    Elements parts;

    // Before the cursor
    if (cursor_pos_ > 0) {
      parts.push_back(text(buffer_.substr(0, cursor_pos_)));
    }

    // Character at cursor (or a space if at the end)
    if (cursor_pos_ < buffer_.size()) {
      parts.push_back(text(std::string(1, buffer_[cursor_pos_])) | inverted);
    } else {
      parts.push_back(text(" ") | inverted); // fake cursor at end
    }

    // After the cursor
    if (cursor_pos_ < buffer_.size()) {
      parts.push_back(text(buffer_.substr(cursor_pos_ + 1)));
    }

    return hbox(std::move(parts)) | bgcolor(Color::Black) | color(Color::Green);
  }

  bool OnEvent(ftxui::Event event) override {
    using namespace ftxui;
    if (event.is_character()) {
      InsertChar(event.character()[0]);
      return true;
    }
    if (event == Event::Backspace) {
      DeleteCharBefore();
      return true;
    }
    if (event == Event::Delete) {
      DeleteCharAfter();
      return true;
    }
    if (event == Event::ArrowLeft) {
      MoveCursorLeft();
      return true;
    }
    if (event == Event::ArrowRight) {
      MoveCursorRight();
      return true;
    }
    // ctrl + left
    if (event == Event::Special({27, 91, 49, 59, 53, 68})) {
      MoveCursorWordLeft();
      return true;
    }
    // ctrl + right
    if (event == Event::Special({27, 91, 49, 59, 53, 67})) {
      MoveCursorWordRight();
      return true;
    }
    // ctrl + w or ctrl + del
    if (event == Event::Special({23}) ||
        event == Event::Special({27, 91, 51, 59, 53, 126})) {
      DeleteWordAfter();
      return true;
    }
    // ctrl + b
    if (event == Event::Special({2})) {
      DeleteWordBefore();
      return true;
    }
    if (event == Event::ArrowUp) {
      HistoryPrev();
      return true;
    }
    if (event == Event::ArrowDown) {
      HistoryNext();
      return true;
    }
    if (event == Event::Return) {
      Accept();
      return true;
    }
    if (event == Event::Escape) {
      Cancel();
      return true;
    }
    return false;
  }

  std::function<void(std::string)> on_submit;
  std::function<void()> on_cancel;

private:
  std::string buffer_;
  size_t cursor_pos_ = 0;
  std::vector<std::string> history_;
  int history_index_ = -1;

  // --- Word boundary and position checks ---
  inline bool AtBeginningOfLine() const { return cursor_pos_ == 0; }
  inline bool AtEndOfLine() const { return cursor_pos_ >= buffer_.size(); }
  inline bool AtBeginningOfWord() const {
    return cursor_pos_ > 0 && buffer_[cursor_pos_ - 1] == ' ' &&
           cursor_pos_ < buffer_.size() && buffer_[cursor_pos_] != ' ';
  }
  inline bool AtEndOfWord() const {
    return cursor_pos_ > 0 && buffer_[cursor_pos_ - 1] != ' ' &&
           (cursor_pos_ >= buffer_.size() || buffer_[cursor_pos_] == ' ');
  }
  inline bool InsideWord() const {
    return cursor_pos_ > 0 && cursor_pos_ < buffer_.size() &&
           buffer_[cursor_pos_] != ' ' && buffer_[cursor_pos_ - 1] != ' ';
  }
  inline bool BetweenBlanks() const {
    return cursor_pos_ > 0 && cursor_pos_ < buffer_.size() &&
           buffer_[cursor_pos_ - 1] == ' ' && buffer_[cursor_pos_] == ' ';
  }

  // --- Cursor motion helpers ---
  size_t findPreviousWordStart(size_t pos) const {
    pos = skipSpacesBackward(pos);
    pos = skipCharacterBackward(pos);
    return pos;
  }

  size_t findNextWordEnd(size_t pos) const {
    pos = skipSpacesForward(pos);
    pos = skipCharactersForward(pos);
    return pos;
  }

  size_t findPreviousWordEnd(size_t pos) const {
    pos = skipCharacterBackward(pos);
    pos = skipSpacesBackward(pos);
    return pos;
  }

  size_t findNextWordStart(size_t pos) const {
    pos = skipCharactersForward(pos);
    pos = skipSpacesForward(pos);
    return pos;
  }

  size_t countSpacesBefore(size_t pos) const {
    auto temp = skipSpacesBackward(pos);
    return pos - temp;
  }

  size_t countSpacesAfter(size_t pos) const {
    auto temp = skipSpacesForward(pos);
    return temp - pos;
  }

  size_t skipCharactersForward(size_t pos) const {
    while (pos < buffer_.size() && buffer_[pos] != ' ')
      ++pos;
    return pos;
  }

  size_t skipCharacterBackward(size_t pos) const {
    while (pos > 0 && buffer_[pos - 1] != ' ')
      --pos;
    return pos;
  }
  size_t skipSpacesForward(size_t pos) const {
    while (pos < buffer_.size() && buffer_[pos] == ' ')
      ++pos;
    return pos;
  }

  size_t skipSpacesBackward(size_t pos) const {
    while (pos > 0 && buffer_[pos - 1] == ' ')
      --pos;
    return pos;
  }

  // --- Core editing ops ---
  void InsertChar(char c) {
    buffer_.insert(cursor_pos_, 1, c);
    ++cursor_pos_;
  }

  void DeleteCharBefore() {
    if (cursor_pos_ > 0) {
      buffer_.erase(cursor_pos_ - 1, 1);
      --cursor_pos_;
    }
  }

  void DeleteCharAfter() {
    if (cursor_pos_ < buffer_.size()) {
      buffer_.erase(cursor_pos_, 1);
    }
  }

  void MoveCursorLeft() {
    if (cursor_pos_ > 0)
      --cursor_pos_;
  }

  void MoveCursorRight() {
    if (cursor_pos_ < buffer_.size())
      ++cursor_pos_;
  }

  void MoveCursorWordLeft() {
    cursor_pos_ = findPreviousWordStart(cursor_pos_);
  }

  void MoveCursorWordRight() { cursor_pos_ = findNextWordEnd(cursor_pos_); }

  // if at begining of line > do nothing
  // if at begining of word > delete all spaces up to the end of previous word
  // if at middle of a word > delete until begin of current (previous) word
  // if at middle of spaces > delete until end of previous word
  void DeleteWordBefore() {
    if (AtBeginningOfLine())
      return;

    auto del_pos = cursor_pos_;

    if (AtBeginningOfWord() || AtEndOfLine()) {
      del_pos = findPreviousWordEnd(del_pos);
    } else if (InsideWord()) {
      del_pos = findPreviousWordStart(del_pos);
    } else if (BetweenBlanks()) {
      del_pos = findPreviousWordEnd(del_pos);
    } else if (AtEndOfWord()) {
      del_pos = findPreviousWordStart(del_pos);
    }

    buffer_.erase(del_pos, cursor_pos_ - del_pos);
    cursor_pos_ = del_pos;
  }

  // if at end of line > do nothing
  // if at end of word > delete all spaces up to the begining of next word
  // if at middle of a word > delete until end of current (next) word
  // if at middle of spaces > delete until end of next word
  void DeleteWordAfter() {
    if (AtEndOfLine())
      return;

    auto del_pos = cursor_pos_;
    if (AtEndOfWord() || AtBeginningOfLine()) {
      del_pos = findNextWordStart(del_pos);
    } else if (InsideWord()) {
      del_pos = findNextWordEnd(del_pos);
    } else if (BetweenBlanks()) {
      del_pos = findNextWordStart(del_pos);
    } else if (AtBeginningOfWord()) {
      del_pos = findNextWordEnd(del_pos);
    }

    buffer_.erase(cursor_pos_, del_pos - cursor_pos_);
  }

  void DeleteToEndOfLine() { buffer_.erase(cursor_pos_); }

  void DeleteToBeginningOfLine() {
    buffer_.erase(0, cursor_pos_);
    cursor_pos_ = 0;
  }

  void Accept() {
    if (on_submit)
      on_submit(buffer_);
    if (buffer_ != "") {
      history_.push_back(buffer_);
    }
    buffer_.clear();
    cursor_pos_ = 0;
    history_index_ = -1;
  }

  void Cancel() {
    if (on_cancel)
      on_cancel();
    buffer_.clear();
    cursor_pos_ = 0;
    history_index_ = -1;
  }

  void HistoryPrev() {
    if (history_.empty())
      return;
    if (history_index_ < static_cast<int>(history_.size()) - 1)
      history_index_++;
    buffer_ = history_[history_.size() - 1 - history_index_];
    cursor_pos_ = buffer_.size();
  }

  void HistoryNext() {
    if (history_index_ <= 0) {
      buffer_.clear();
      cursor_pos_ = 0;
      history_index_ = -1;
      return;
    }
    history_index_--;
    buffer_ = history_[history_.size() - 1 - history_index_];
    cursor_pos_ = buffer_.size();
  }
};
