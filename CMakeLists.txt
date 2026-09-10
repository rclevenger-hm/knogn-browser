cmake_minimum_required(VERSION 3.22)
project(KnognBrowser VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)
include(FetchContent)

find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets WebEngineWidgets WebEngineCore)
qt_standard_project_setup()

# Passwords must use a platform secure store. Build QtKeychain statically so the
# release packages do not gain an extra runtime library while preserving native
# Windows Credential Store / macOS Keychain / Linux keyring backends.
set(BUILD_TRANSLATIONS OFF CACHE BOOL "" FORCE)
set(BUILD_TEST_APPLICATION OFF CACHE BOOL "" FORCE)
set(BUILD_QTQUICK_DEMO OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(LIBSECRET_SUPPORT OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    qtkeychain
    GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
    GIT_TAG 0.17.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(qtkeychain)

qt_add_executable(knogn-browser
    src/main.cpp
    src/appsettings.cpp
    src/appsettings.h
    src/bookmarks.cpp
    src/bookmarks.h
    src/branding.cpp
    src/branding.h
    src/browserflags.cpp
    src/browserflags.h
    src/browserpage.cpp
    src/browserpage.h
    src/browserwindow.cpp
    src/browserwindow.h
    src/credentialstore.cpp
    src/credentialstore.h
    src/identitydetails.cpp
    src/identitydetails.h
    src/mediadetails.cpp
    src/mediadetails.h
    src/privacyinterceptor.cpp
    src/privacyinterceptor.h
    src/privacyprofile.cpp
    src/privacyprofile.h
    src/settingsdialog.cpp
    src/settingsdialog.h
)

target_link_libraries(knogn-browser PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
    qt6keychain
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
    set(KNOGN_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/knogn.ico")
    set(KNOGN_MANIFEST_PATH "${CMAKE_CURRENT_BINARY_DIR}/knogn.manifest")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/knogn.manifest.in"
        "${KNOGN_MANIFEST_PATH}"
        @ONLY
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/knogn.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/knogn.rc"
        @ONLY
    )
    target_sources(knogn-browser PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/knogn.rc")
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
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/knogn.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/knogn.ico")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\Knogn.exe")
    set(CPACK_PACKAGE_EXECUTABLES "Knogn" "Knogn")
    set(CPACK_CREATE_DESKTOP_LINKS "Knogn")
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
