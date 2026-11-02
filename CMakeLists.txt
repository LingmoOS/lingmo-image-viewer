# SPDX-FileCopyrightText: 2026 LingmoOS Contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

project(lingmo-image-viewer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CTest)

find_package(LingmoSDK REQUIRED)

# The folder listing, formatting and rotation: no window, no toolkit.
add_library(viewer-core STATIC
    src/imagelist.cpp
    src/rotation.cpp
)
target_include_directories(viewer-core PUBLIC src)

# The window, the bars and the picture area.
add_library(viewer-ui STATIC
    src/canvas.cpp
    src/glyphs.cpp
    src/viewerwindow.cpp
    src/widgets.cpp
)
target_include_directories(viewer-ui PUBLIC src)
target_link_libraries(viewer-ui PUBLIC viewer-core Lingmo::SDK)

add_executable(${PROJECT_NAME} src/main.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE IMAGE_VIEWER_VERSION="${PROJECT_VERSION}")
target_link_libraries(${PROJECT_NAME} PRIVATE viewer-ui)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES icons/lingmo-image-viewer.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

lingmo_add_translations(${PROJECT_NAME} SOURCES src DESKTOP_FILES lingmo-image-viewer.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lingmo-image-viewer.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
