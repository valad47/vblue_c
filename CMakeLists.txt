cmake_minimum_required(VERSION 3.20)

project(vblue_c)

add_subdirectory(bluez_inc)

find_package(raylib)
find_package(PkgConfig REQUIRED)
pkg_check_modules(glib REQUIRED IMPORTED_TARGET glib-2.0)

add_executable(vblue src/main.c)
set_property(TARGET vblue PROPERTY C_STANDARD 23)
target_link_libraries(vblue PRIVATE raylib Binc)
