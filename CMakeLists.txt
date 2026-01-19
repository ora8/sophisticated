cmake_minimum_required(VERSION 3.16)
project(SophisticatedGTKAppGTK4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)

add_executable(sophisticated
  src/main.cpp
  src/app_window.cpp
  src/find_text_dialog.cpp
)

target_include_directories(sophisticated PRIVATE
  ${GTKMM_INCLUDE_DIRS}
  src
)

target_compile_options(sophisticated PRIVATE
  ${GTKMM_CFLAGS_OTHER}
  -Wall -Wextra -Wpedantic
)

target_link_libraries(sophisticated PRIVATE
  ${GTKMM_LIBRARIES}
)
