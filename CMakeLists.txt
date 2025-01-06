cmake_minimum_required(VERSION 3.23)
project(Fichillsh)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(PROJECT_CLIENT ${PROJECT_NAME}_client)
set(PROJECT_SERVER ${PROJECT_NAME}_server)

#Check for architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH 64)
else()
    set(ARCH 32)
endif()

find_package(FastEngine REQUIRED)

#Fetch GRUpdater
FetchContent_Declare(
        GRUpdater
        GIT_REPOSITORY https://github.com/JonathSpirit/GRUpdater.git
        GIT_TAG master
)

FetchContent_MakeAvailable(GRUpdater)

#Fetch box2d
FetchContent_Declare(
        box2d
        GIT_REPOSITORY https://github.com/erincatto/box2d.git
        GIT_TAG f0763aca7d4dfa8f1c19c2c4a9e66fa6e93ea32e
)

set(BOX2D_SAMPLES OFF)
set(BOX2D_BENCHMARKS OFF)
set(BOX2D_DOCS OFF)
set(BOX2D_PROFILE OFF)
set(BOX2D_VALIDATE ON)
set(BOX2D_UNIT_TESTS OFF)

FetchContent_MakeAvailable(box2d)

set(CMAKE_DEBUG_POSTFIX "${ARCH}_d")
set(CMAKE_RELEASE_POSTFIX "${ARCH}")

#Executable
add_executable(${PROJECT_CLIENT})
target_sources(${PROJECT_CLIENT} PRIVATE client/main.cpp)
target_sources(${PROJECT_CLIENT} PRIVATE client/player.cpp client/player.hpp)
target_sources(${PROJECT_CLIENT} PRIVATE client/game.cpp client/game.hpp)
target_sources(${PROJECT_CLIENT} PRIVATE client/fish.cpp client/fish.hpp)
target_sources(${PROJECT_CLIENT} PRIVATE client/ducky.cpp client/ducky.hpp)

add_executable(${PROJECT_SERVER})
target_sources(${PROJECT_SERVER} PRIVATE server/main.cpp)
target_sources(${PROJECT_SERVER} PRIVATE server/player.cpp server/player.hpp)

target_sources(${PROJECT_SERVER} PRIVATE share/network.hpp)

target_sources(${PROJECT_CLIENT} PRIVATE share/network.hpp)

#Dependencies
add_dependencies(${PROJECT_CLIENT} box2d GRUpdater)

target_link_libraries(${PROJECT_CLIENT} PRIVATE SDL2::SDL2main FastEngine::FastEngine)
target_link_libraries(${PROJECT_CLIENT} PRIVATE box2d GRUpdater)

target_include_directories(${PROJECT_CLIENT} PRIVATE ${GRUpdater_SOURCE_DIR})

target_link_libraries(${PROJECT_SERVER} PRIVATE SDL2::SDL2main FastEngine::FastEngineServer)

#Resources
add_custom_command(TARGET ${PROJECT_CLIENT} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/resources
        ${CMAKE_CURRENT_BINARY_DIR}/resources
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/resources
        VERBATIM)

if(WIN32)
    set(FGE_LINK_SERVER_LIBRARIES "FastEngine::FastEngineServer;SDL2::SDL2;SDL2_image::SDL2_image")
    set(FGE_LINK_CLIENT_LIBRARIES "FastEngine::FastEngine;SDL2::SDL2;SDL2_image::SDL2_image;SDL2_mixer::SDL2_mixer;GRUpdater")

    foreach (FGE_LINK_LIBRARY ${FGE_LINK_CLIENT_LIBRARIES})
        add_custom_command(TARGET ${PROJECT_CLIENT} PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${FGE_LINK_LIBRARY}>
                $<TARGET_FILE_DIR:${PROJECT_CLIENT}>)
    endforeach()

    foreach (FGE_LINK_LIBRARY ${FGE_LINK_SERVER_LIBRARIES})
        add_custom_command(TARGET ${PROJECT_SERVER} PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${FGE_LINK_LIBRARY}>
                $<TARGET_FILE_DIR:${PROJECT_SERVER}>)
    endforeach()
endif()