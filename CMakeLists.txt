# This CMake file should build SuperHaxagon on MinGW and MSVC
# It will compile the PSP version if used through psp-cmake
# It will not compile the 3DS version, use the Makefile for that
cmake_minimum_required(VERSION 3.10)
project(Super_Haxagon)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wall -Wextra -pedantic)
endif(CMAKE_COMPILER_IS_GNUCXX)

if(UNIX)
    message(STATUS "Compiling with GCC")
    set(DRIVER source/Driver/Linux/PlatformLinux.cpp)
    # Make sure you have SFML installed via your package manager
    # Usually `sudo apt install libsfml-dev` or similar (Debian/Ubuntu)
endif()

if(MINGW OR MSYS)
    message(STATUS "Compiling with MinGW")
    set(DRIVER source/Driver/Win/PlatformWin.cpp)
    set(SFML_STATIC_LIBRARIES TRUE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
    set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries/SFML-MinGW/lib/cmake/SFML")
endif()

if(MSVC)
    message(STATUS "Compiling with MSVC")
    set(DRIVER source/Driver/Win/PlatformWin.cpp)
    set(SFML_STATIC_LIBRARIES TRUE)
    set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries/SFML-MSVC/lib/cmake/SFML")
endif()

if(PSP)
    message(STATUS "Compiling PSP version")
    set(DRIVER source/Driver/PSP/AudioLoaderPSP.cpp source/Driver/PSP/AudioPlayerPSP.cpp source/Driver/PSP/FontPSP.cpp source/Driver/PSP/PlatformPSP.cpp)
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
if(NOT PSP)
    include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../Ndless/ndless-sdk/include")
    include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/libraries/nGL")
    include_directories(SYSTEM "$ENV{DEVKITPRO}/libctru/include")
    include_directories(SYSTEM "$ENV{DEVKITPRO}/libnx/include")
    include_directories(SYSTEM "$ENV{DEVKITPRO}/portlibs/switch/include")
    include_directories(SYSTEM "$ENV{DEVKITPRO}/portlibs/switch/include/freetype2")
    include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/libraries/stb")
    
    find_package(SFML 2 COMPONENTS system window graphics audio)
endif()

add_executable(SuperHaxagon WIN32 ${DRIVER}
    source/States/Load.cpp
    source/States/Menu.cpp
    source/States/Over.cpp
    source/States/Play.cpp
    source/States/Quit.cpp
    source/States/Transition.cpp
    source/States/Win.cpp

    source/Factories/LevelFactory.cpp
    source/Factories/PatternFactory.cpp
    source/Factories/WallFactory.cpp

    source/Objects/Level.cpp
    source/Objects/Pattern.cpp
    source/Objects/Wall.cpp

    source/Core/Platform.cpp
    source/Core/Game.cpp
    source/Core/Metadata.cpp
    source/Core/Main.cpp
    source/Core/Structs.cpp)

if(SFML_FOUND)
    target_sources(SuperHaxagon PRIVATE
        source/Driver/SFML/AudioLoaderSFML.cpp
        source/Driver/SFML/AudioPlayerSoundSFML.cpp
        source/Driver/SFML/AudioPlayerMusicSFML.cpp
        source/Driver/SFML/FontSFML.cpp
        source/Driver/SFML/PlatformSFML.cpp)
endif()

if(PSP)
    target_link_libraries(SuperHaxagon pspaudio pspaudiolib pspctrl pspdebug pspdisplay pspge pspgu psppower)
    target_compile_options(SuperHaxagon PRIVATE -O2 -g0)
else()
    target_link_libraries(SuperHaxagon sfml-graphics sfml-window sfml-audio sfml-system)
endif()

if(MINGW OR MSYS OR MSVC)
    # Only need to copy dll if on windows
    add_custom_command(TARGET SuperHaxagon POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SFML_DIR}/../../../bin/openal32.dll $<TARGET_FILE_DIR:SuperHaxagon>)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openal32.dll DESTINATION .)
endif()

if(PSP)
    set(PSP_DIR ${CMAKE_CURRENT_BINARY_DIR}/PSP/GAME/SuperHaxagon)
    add_custom_command(TARGET SuperHaxagon PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${PSP_DIR}/romfs)
    add_custom_command(TARGET SuperHaxagon PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/romfs/levels.haxagon ${PSP_DIR}/romfs)
    add_custom_command(TARGET SuperHaxagon PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/romfs/bump-it-up.ttf ${PSP_DIR}/romfs)
    # TODO: Only copy files needed in the PSP version.
    create_pbp_file(
        TARGET SuperHaxagon
        TITLE "Super Haxagon"
        VERSION ${PROJECT_VERSION}
        OUTPUT_DIR ${PSP_DIR}
        ICON_PATH ${PROJECT_SOURCE_DIR}/media/psp_icon0.png
        BACKGROUND_PATH ${PROJECT_SOURCE_DIR}/media/psp_pic1.png
        #MUSIC_PATH ${PROJECT_SOURCE_DIR}/media/psp_snd0.at3
	)#BUILD_PRX ENC_PRX) # TODO DEBUG: comment-in
else()
    add_custom_command(TARGET SuperHaxagon POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/romfs $<TARGET_FILE_DIR:SuperHaxagon>/romfs)
    install(TARGETS SuperHaxagon RUNTIME DESTINATION .)
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/romfs DESTINATION .)
endif()
