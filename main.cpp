#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "line_editor.hpp" // Include your custom LineEditor

using namespace ftxui;

enum class Mode { NORMAL, COMMAND };

int main() {
  auto screen = ScreenInteractive::Fullscreen();

  Mode mode = Mode::NORMAL;
  std::string status = "NORMAL mode — press ':' to enter command mode.";
  std::string last_command;

  // Create the LineEditor instance
  auto line_editor = std::make_shared<LineEditor>();

  // Connect hooks
  line_editor->on_submit = [&](std::string cmd) {
    last_command = cmd;
    mode = Mode::NORMAL;
    status = "NORMAL mode — submitted: " + cmd;
    line_editor->Clear();
  };

  line_editor->on_cancel = [&]() {
    mode = Mode::NORMAL;
    status = "NORMAL mode — cancelled command.";
    line_editor->Clear();
  };

  // Render the full UI
  Component ui = Renderer(line_editor, [&] {
    Elements content;
    content.push_back(text("📝 Vim-like App (FTXUI)") | bold | center);
    content.push_back(separator());
    content.push_back(text("Status: " + status));
    if (!last_command.empty()) {
      content.push_back(text("Last command: " + last_command));
    }

    content.push_back(filler());

    if (mode == Mode::COMMAND) {
      content.push_back(separator());
      content.push_back(hbox({text(":") | bold, line_editor->Render()}));
    } else {
      content.push_back(text("Press ':' to enter command mode.") | dim);
    }

    return vbox(std::move(content)) | border;
  });

  // Attach event handler
  Component app = CatchEvent(ui, [&](Event event) {
    if (mode == Mode::NORMAL && event.character() == ":") {
      mode = Mode::COMMAND;
      status = "COMMAND mode — type and press Enter.";
      return true;
    }

    if (mode == Mode::COMMAND) {
      return line_editor->OnEvent(event);
    }

    return true;
  });

  screen.Loop(app);
  return 0;
}
