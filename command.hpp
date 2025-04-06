#pragma once

/*#include <functional>*/
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

struct CommandDefinition {
  std::string name;
  std::vector<std::string> positional_args;
  std::set<std::string> allowed_flags;

  // Completion functions per argument index
  /*std::function<std::vector<std::string>(size_t arg_index,*/
  /*                                       const std::string &input)>*/
  /*    completion_callback;*/
};

struct Command {
  std::string name;
  std::map<std::string, std::string> args;
  std::set<std::string> flags;
};

inline std::vector<std::string> tokenize(const std::string &input) {
  std::vector<std::string> tokens;
  std::string current;
  bool in_quotes = false;

  for (char c : input) {
    if (c == '"') {
      in_quotes = !in_quotes;
      continue;
    }
    if (std::isspace(c) && !in_quotes) {
      if (!current.empty()) {
        tokens.push_back(current);
        current.clear();
      }
    } else {
      current += c;
    }
  }
  if (!current.empty())
    tokens.push_back(current);
  return tokens;
}

inline std::optional<Command>
parse_command(const std::vector<std::string> &tokens,
              const std::map<std::string, CommandDefinition> &registry) {
  if (tokens.empty())
    return std::nullopt;

  std::string name = tokens[0];
  auto it = registry.find(name);
  if (it == registry.end())
    return std::nullopt;

  Command cmd;
  cmd.name = name;

  const auto &def = it->second;
  size_t arg_index = 0;

  for (size_t i = 1; i < tokens.size(); ++i) {
    const std::string &token = tokens[i];
    if (token.starts_with("--")) {
      if (def.allowed_flags.count(token)) {
        cmd.flags.insert(token);
      } else {
        std::cerr << "Unknown flag: " << token << "\n";
        return std::nullopt;
      }
    } else {
      if (arg_index < def.positional_args.size()) {
        cmd.args[def.positional_args[arg_index]] = token;
        ++arg_index;
      } else {
        std::cerr << "Unexpected positional argument: " << token << "\n";
        return std::nullopt;
      }
    }
  }

  return cmd;
}
