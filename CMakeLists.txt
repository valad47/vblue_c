cmake_minimum_required(VERSION 3.20)

project(vblue_c)

add_subdirectory(bluez_inc)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_executable(vblue src/main.c)
set_property(TARGET vblue PROPERTY C_STANDARD 23)
target_link_libraries(vblue PRIVATE PkgConfig::GTK4 Binc)