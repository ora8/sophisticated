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
  src/replace_text_dialog.cpp
)

target_include_directories(sophisticated PRIVATE
  ${GTKMM_INCLUDE_DIRS}
  src
)

target_compile_options(sophisticated PRIVATE
  ${GTKMM_CFLAGS_OTHER}
)

if (MSVC)
  # MSVC warning level + “treat warnings as errors” (optional)
  target_compile_options(sophisticated PRIVATE
    /W4           # similar to -Wall/-Wextra (not identical)
    /permissive-  # stricter standard conformance (roughly like pedantic-ish)
    /EHsc         # standard C++ exceptions (often needed)
    /utf-8        # make source files UTF-8
    # /WX         # uncomment if you want warnings as errors
  )
else()
  target_compile_options(sophisticated PRIVATE
    -Wall -Wextra -Wpedantic
  )
endif()


#target_link_libraries(sophisticated PRIVATE
#  ${GTKMM_LIBRARIES}
#)

# --------------------------
# GTKMM dependency
# --------------------------
find_package(PkgConfig REQUIRED)

# gtkmm 4
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

target_link_libraries(sophisticated PRIVATE PkgConfig::GTKMM)
