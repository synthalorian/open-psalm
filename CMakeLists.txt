cmake_minimum_required(VERSION 3.16)
project(open_psalm VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FTXUI
include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v5.0.0
)
FetchContent_MakeAvailable(ftxui)

# Find system packages
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(SQLITE3 IMPORTED_TARGET sqlite3)
  pkg_check_modules(NLOHMANN IMPORTED_TARGET nlohmann_json)
endif()

if(NOT SQLITE3_FOUND)
  find_library(SQLITE3_LIBRARY sqlite3 REQUIRED)
  find_path(SQLITE3_INCLUDE_DIR sqlite3.h REQUIRED)
  add_library(sqlite3_lib UNKNOWN IMPORTED)
  set_target_properties(sqlite3_lib PROPERTIES
    IMPORTED_LOCATION "${SQLITE3_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE_DIR}"
  )
endif()

if(NOT NLOHMANN_FOUND)
  find_path(NLOHMANN_INCLUDE_DIR nlohmann/json.hpp REQUIRED)
else()
  # pkg_check_modules sets NLOHMANN_INCLUDE_DIRS (plural)
  set(NLOHMANN_INCLUDE_DIR ${NLOHMANN_INCLUDE_DIRS})
endif()

set(SOURCES
  src/main.cpp
  src/database.cpp
  src/theme.cpp
  src/app.cpp
  src/config.cpp
  src/navigation.cpp
  src/rendering.cpp
)

add_executable(open-psalm ${SOURCES})
target_include_directories(open-psalm PRIVATE src ${NLOHMANN_INCLUDE_DIR})

target_link_libraries(open-psalm PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
)

if(SQLITE3_FOUND)
  target_link_libraries(open-psalm PRIVATE PkgConfig::SQLITE3)
else()
  target_link_libraries(open-psalm PRIVATE sqlite3_lib)
endif()

# Add fts5 extension to sqlite
target_compile_definitions(open-psalm PRIVATE SQLITE_ENABLE_FTS5)

# ============================================================
# Tests
# ============================================================
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
  # Fetch Catch2
  include(FetchContent)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v3.7.1
  )
  FetchContent_MakeAvailable(Catch2)

  set(TEST_SOURCES
    tests/test_database.cpp
    tests/test_app.cpp
    src/database.cpp
    src/theme.cpp
    src/config.cpp
    src/navigation.cpp
    src/rendering.cpp
    src/app.cpp
  )

  add_executable(open-psalm-tests ${TEST_SOURCES})
  target_include_directories(open-psalm-tests PRIVATE src ${NLOHMANN_INCLUDE_DIR})
  target_link_libraries(open-psalm-tests PRIVATE
    Catch2::Catch2WithMain
    ftxui::dom
    ftxui::screen
    ftxui::component
  )

  if(SQLITE3_FOUND)
    target_link_libraries(open-psalm-tests PRIVATE PkgConfig::SQLITE3)
  else()
    target_link_libraries(open-psalm-tests PRIVATE sqlite3_lib)
  endif()

  target_compile_definitions(open-psalm-tests PRIVATE SQLITE_ENABLE_FTS5)

  # Register tests with CTest
  include(CTest)
  enable_testing()
  add_test(NAME open-psalm-tests COMMAND open-psalm-tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
