cmake_minimum_required(VERSION 3.12)
project(knowBitApp LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_library(RAYLIB_LIB raylib PATHS /opt/homebrew/lib)
include_directories(/opt/homebrew/include)

add_executable(knowBitApp
    main.cpp
    loginPage.cpp
    dashboard.cpp
    calendar.cpp
    task.cpp
    suyans.cpp
    arohaMain.cpp
)

if(APPLE)
    find_library(OPENGL_FRAMEWORK OpenGL)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)

    target_link_libraries(knowBitApp PRIVATE
        ${RAYLIB_LIB}
        ${OPENGL_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
    )
else()
    target_link_libraries(knowBitApp PRIVATE raylib)
endif()
