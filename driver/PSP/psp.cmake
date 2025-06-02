# SPDX-FileCopyrightText: 2025-2026 Sotiris Papatheodorou
# SPDX-License-Identifier: GPL-3.0-or-later

message(STATUS "Building for the PSP...")

set(PLATFORM_NAME "PSP")

set(DRIVER
    driver/All/source/PlatformIsFullyRandom.cpp
    driver/All/source/PlatformSupportsFilesystem.cpp
    driver/PSP/source/AudioBufferPSP.cpp
    driver/PSP/source/AudioFilePSP.cpp
    driver/PSP/source/AudioFileVorbisPSP.cpp
    driver/PSP/source/AudioFileWavPSP.cpp
    driver/PSP/source/CommonPSP.cpp
    driver/PSP/source/ControlsPSP.cpp
    driver/PSP/source/FontPSP.cpp
    driver/PSP/source/MusicPSP.cpp
    driver/PSP/source/PlatformPSP.cpp
    driver/PSP/source/ScreenPSP.cpp
    driver/PSP/source/SoundPSP.cpp
)

find_package(Vorbis REQUIRED)

add_executable(${PROJECT_NAME} ${DRIVER} ${SOURCES})
target_link_libraries(${PROJECT_NAME} Vorbis::vorbisfile pspaudio pspaudiolib pspctrl pspdebug pspdisplay pspge pspgu pspreg)
target_compile_options(${PROJECT_NAME} PRIVATE -O2 -g0)

# I find the menu music somewhat annoying but comment-in the MUSIC_PATH line if
# you want it.
create_pbp_file(
	TARGET ${PROJECT_NAME}
	TITLE "Super Haxagon"
	ICON_PATH ${CMAKE_CURRENT_LIST_DIR}/resources/psp_icon0.png
	BACKGROUND_PATH ${CMAKE_CURRENT_LIST_DIR}/resources/psp_pic1.png
	#MUSIC_PATH ${CMAKE_CURRENT_LIST_DIR}/resources/psp_snd0.at3
	VERSION ${PROJECT_VERSION}
	OUTPUT_DIR ${CMAKE_BINARY_DIR}
	BUILD_PRX ENC_PRX)

string(TOUPPER "${PROJECT_NAME}" GAME_NAME)
set(GAME_DIR "${GAME_NAME}")
set(GAME_DATA_DIR "${GAME_DIR}/DATA")
install(FILES "${CMAKE_BINARY_DIR}/EBOOT.PBP" DESTINATION "${GAME_DIR}")
install(FILES "${CMAKE_CURRENT_LIST_DIR}/resources/psp_PromptFont.ttf" DESTINATION "${GAME_DATA_DIR}")
install(FILES
	"${CMAKE_SOURCE_DIR}/romfs/bump-it-up.ttf"
	"${CMAKE_SOURCE_DIR}/romfs/levels.haxagon"
	DESTINATION "${GAME_DATA_DIR}")
install(DIRECTORY
	"${CMAKE_SOURCE_DIR}/romfs/bgm"
	"${CMAKE_SOURCE_DIR}/romfs/sound"
	DESTINATION "${GAME_DATA_DIR}")
install(DIRECTORY DESTINATION "${GAME_DIR}/USER")

set(LICENSE_PATH "${GAME_DIR}/LICENSES")
set(LICENSE_DRIVER "${CMAKE_CURRENT_LIST_DIR}/LICENSE.md")
set(README_PATH "${GAME_DIR}")
set(README_DRIVER "${CMAKE_CURRENT_LIST_DIR}/INSTALL.md")
