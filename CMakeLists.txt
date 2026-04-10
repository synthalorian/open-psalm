cmake_minimum_required(VERSION 3.16)
project(open_psalm VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FTXUI
include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v5.0.0
)
FetchContent_MakeAvailable(ftxui)

add_executable(open-psalm src/main.cpp)
target_link_libraries(open-psalm PRIVATE ftxui::screen ftxui::dom ftxui::component)
target_include_directories(open-psalm PRIVATE src)
