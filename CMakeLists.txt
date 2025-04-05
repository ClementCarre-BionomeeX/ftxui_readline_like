
cmake_minimum_required(VERSION 3.16)
project(vim_like_tui)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FTXUI assumes it's added as a subdirectory or available via FetchContent
include(FetchContent)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG        origin/main
)

FetchContent_MakeAvailable(ftxui)

add_executable(vim_like_tui main.cpp)
target_link_libraries(vim_like_tui PRIVATE ftxui::screen ftxui::dom ftxui::component readline)
