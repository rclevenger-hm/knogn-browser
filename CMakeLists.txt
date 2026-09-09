cmake_minimum_required(VERSION 3.22)
project(KnognBrowser VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets WebEngineWidgets WebEngineCore)
qt_standard_project_setup()

qt_add_executable(knogn-browser
    src/main.cpp
    src/browserflags.cpp
    src/browserflags.h
    src/browserpage.cpp
    src/browserpage.h
    src/browserwindow.cpp
    src/browserwindow.h
    src/mediadetails.cpp
    src/mediadetails.h
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

set_target_properties(knogn-browser PROPERTIES
    OUTPUT_NAME "Knogn"
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
    set_target_properties(knogn-browser PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.knogn.browser"
        MACOSX_BUNDLE_BUNDLE_NAME "Knogn"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
endif()

install(TARGETS knogn-browser
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET knogn-browser
    OUTPUT_SCRIPT knogn_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_TRANSLATIONS
)
install(SCRIPT ${knogn_deploy_script})

set(CPACK_PACKAGE_NAME "Knogn")
set(CPACK_PACKAGE_VENDOR "Knogn")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast by omission. Private by design.")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Knogn")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_VERBATIM_VARIABLES ON)

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Knogn")
    set(CPACK_NSIS_PACKAGE_NAME "Knogn")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_PACKAGE_EXECUTABLES "Knogn" "Knogn")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;ZIP")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;TXZ")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/knogn")
    set(CPACK_STRIP_FILES TRUE)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Roger Clevenger")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
    set(CPACK_DEBIAN_PACKAGE_SECTION "web")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6")
endif()

include(CPack)
