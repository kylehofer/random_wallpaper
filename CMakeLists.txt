cmake_minimum_required(VERSION 3.15)
project(random_wallpaper)

set(CMAKE_CXX_STANDARD 17)
find_package(OpenCV REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Gtkmm REQUIRED IMPORTED_TARGET gtkmm-4.0)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(random_wallpaper main.cpp)

target_link_libraries(random_wallpaper PkgConfig::Gtkmm ${OpenCV_LIBS})