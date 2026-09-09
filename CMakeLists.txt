cmake_minimum_required(VERSION 3.22)
project(KnognBrowser VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets WebEngineWidgets WebEngineCore)

add_executable(knogn-browser
    src/main.cpp
    src/browserflags.cpp
    src/browserflags.h
    src/browserpage.cpp
    src/browserpage.h
    src/browserwindow.cpp
    src/browserwindow.h
    src/privacyinterceptor.cpp
    src/privacyinterceptor.h
    src/privacyprofile.cpp
    src/privacyprofile.h
)

target_link_libraries(knogn-browser PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
)

target_compile_definitions(knogn-browser PRIVATE
    KNOGN_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
    target_compile_options(knogn-browser PRIVATE /W4 /permissive-)
else()
    target_compile_options(knogn-browser PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    set_target_properties(knogn-browser PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(APPLE)
    set_target_properties(knogn-browser PROPERTIES MACOSX_BUNDLE TRUE)
endif()
