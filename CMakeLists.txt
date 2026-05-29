cmake_minimum_required(VERSION 3.11)
project(Biocenosis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE SOURCES "src/*.cpp")
# Копируем ресурсы в папку с бинарником при сборке
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources" 
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(Biocenosis PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    XCODE_ATTRIBUTE_CUSTOM_LAUNCH_PROPERTY_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Для того чтобы VS Code (через CMake Tools) гарантированно видел правильную папку
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    set_property(TARGET Biocenosis PROPERTY LAUNCH_COMMAND_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} raylib)