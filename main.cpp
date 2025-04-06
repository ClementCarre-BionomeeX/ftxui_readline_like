#include <cstdio>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <map>
#include <optional>

#include "command.hpp"
#include "line_editor.hpp" // Include your custom LineEditor

using namespace ftxui;

enum class Mode { NORMAL, COMMAND };

// CommandRouter
class CommandRouter {
public:
  using Handler = std::function<void(const Command &)>;

  void register_command(const std::string &name, Handler handler) {
    handlers[name] = std::move(handler);
  }

  bool execute(const Command &cmd) const {
    auto it = handlers.find(cmd.name);
    if (it != handlers.end()) {
      it->second(cmd);
      return true;
    }
    return false;
  }

private:
  std::map<std::string, Handler> handlers;
};

int main() {
  ScreenInteractive screen = ScreenInteractive::Fullscreen();
  ScreenInteractive *screen_ptr = &screen;

  Mode mode = Mode::NORMAL;
  std::string status = "NORMAL mode — press ':' to enter command mode.";
  std::string last_command;
  std::optional<Elements> parsed_command;

  std::map<std::string, CommandDefinition> registry = {
      {"open", CommandDefinition{.name = "open",
                                 .positional_args = {"filename"},
                                 .allowed_flags = {"--readonly"}}},
      {"q", CommandDefinition{
                .name = "quit", .positional_args = {}, .allowed_flags = {}}}};

  // Create the LineEditor instance
  auto line_editor = std::make_shared<LineEditor>();

  // Setup command router
  CommandRouter router;
  router.register_command("open", [&](const Command &cmd) {
    Elements tmp_cmd;
    tmp_cmd.push_back(text("Command: open"));
    for (auto &[k, v] : cmd.args) {
      tmp_cmd.push_back(text("Arg: " + k + " = " + v));
    }
    for (auto &flag : cmd.flags) {
      tmp_cmd.push_back(text("Flag: " + flag));
    }
    parsed_command = tmp_cmd;
  });

  router.register_command("quit", [&](const Command &) { screen_ptr->Exit(); });

  // Connect hooks
  line_editor->on_submit = [&](std::string cmd) {
    last_command = cmd;
    mode = Mode::NORMAL;
    status = "NORMAL mode — submitted: " + cmd;

    auto tokens = tokenize(cmd);
    auto parsed = parse_command(tokens, registry);
    parsed_command = std::nullopt;

    if (parsed) {
      if (!router.execute(*parsed)) {
        status = "Unknown command: " + parsed->name;
      }
    }
  };

  line_editor->on_cancel = [&] {
    mode = Mode::NORMAL;
    status = "NORMAL mode — cancelled command.";
    line_editor->Clear();
  };

  // Render the full UI
  Component ui = Renderer(line_editor, [&] {
    Elements content;
    content.push_back(text("🗘 Vim-like App (FTXUI)") | bold | center);
    content.push_back(separator());
    content.push_back(text("Status: " + status));
    if (!last_command.empty()) {
      content.push_back(text("Last command: " + last_command));
    }
    if (parsed_command) {
      for (auto &v : *parsed_command) {
        content.push_back(v);
      }
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
