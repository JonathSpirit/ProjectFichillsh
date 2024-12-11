cmake_minimum_required(VERSION 3.23)
project(Fichillsh)

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

#Libs
#add_subdirectory("libs/box2d" EXCLUDE_FROM_ALL)

#Dependency
#add_dependencies(FastEngine::FastEngineServer gTerminal)

#Libs
#include_directories("libs/include")

set(CMAKE_DEBUG_POSTFIX "${ARCH}_d")
set(CMAKE_RELEASE_POSTFIX "${ARCH}")

#Executable
add_executable(${PROJECT_CLIENT})
target_sources(${PROJECT_CLIENT} PRIVATE client/main.cpp)
target_sources(${PROJECT_CLIENT} PRIVATE client/player.cpp client/player.hpp)
target_link_libraries(${PROJECT_CLIENT} PRIVATE SDL2::SDL2main FastEngine::FastEngine)

#Resources
add_custom_command(TARGET ${PROJECT_CLIENT} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/resources
        ${CMAKE_CURRENT_BINARY_DIR}/resources
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/resources
        VERBATIM)

if(WIN32)
    set(FGE_LINK_SERVER_LIBRARIES "FastEngine::FastEngineServer;SDL2::SDL2;SDL2_image::SDL2_image")
    set(FGE_LINK_CLIENT_LIBRARIES "FastEngine::FastEngine;SDL2::SDL2;SDL2_image::SDL2_image;SDL2_mixer::SDL2_mixer")

    foreach (FGE_LINK_LIBRARY ${FGE_LINK_CLIENT_LIBRARIES})
        add_custom_command(TARGET ${PROJECT_CLIENT} PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${FGE_LINK_LIBRARY}>
                $<TARGET_FILE_DIR:${PROJECT_CLIENT}>)
    endforeach()
endif()