cmake_minimum_required(VERSION 3.15)

project(mash16 VERSION 2.0 LANGUAGES CXX)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
find_package(OpenGL REQUIRED)
find_package(Git REQUIRED)

set(CMAKE_CXX_STANDARD 20)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
   add_compile_options(-O0 -g -ggdb)
else()
   add_compile_options(-O3)
endif()

execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                RESULT_VARIABLE imgui_git_sm)
if(${imgui_git_sm})
   message("Could not clone one or more Git submodules!")
endif()
execute_process(COMMAND cp ${PROJECT_SOURCE_DIR}/src/imgui_CMakeLists.txt ${PROJECT_SOURCE_DIR}/imgui/CMakeLists.txt
                OUTPUT_VARIABLE cp_imgui_cml)
if(${cp_imgui_cml})
   message("Could not copy custom CMakeLists.txt to imgui/")
endif()
set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/imgui")
add_subdirectory(imgui)

add_compile_options(-mssse3)

if(EXISTS ${PROJECT_SOURCE_DIR}/.git)
   execute_process(COMMAND bash "-c" "${GIT_EXECUTABLE} describe --match \"v*\" | cut -d'-' -f1 | cut -c2-"
                   OUTPUT_VARIABLE mash16_version OUTPUT_STRIP_TRAILING_WHITESPACE)
   execute_process(COMMAND bash "-c" "${GIT_EXECUTABLE} rev-parse --short HEAD"
                   OUTPUT_VARIABLE mash16_build OUTPUT_STRIP_TRAILING_WHITESPACE)
   add_compile_definitions(-DVERSION="${mash16_version}" -DBUILD="${mash16_build}")
endif()

include_directories(${PROJECT_SOURCE_DIR})
include_directories("${PROJECT_SOURCE_DIR}/core")
include_directories("${PROJECT_SOURCE_DIR}/header")
add_subdirectory(src)

add_executable(mash16 src/options.cpp src/strings.cpp src/main.cpp)
add_executable(cpu_test src/options.cpp src/strings.cpp src/cpu_test.cpp)

# Link SDL2
if(TARGET SDL2::SDL2main)
    target_link_libraries(mash16 PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(mash16 PRIVATE mash16_header mash16_core SDL2::SDL2 OpenGL::GL imgui_impl_sdl2 imgui_impl_opengl2)
target_link_libraries(cpu_test PRIVATE mash16_header mash16_core SDL2::SDL2 OpenGL::GL imgui_impl_sdl2 imgui_impl_opengl2)

install(TARGETS mash16 DESTINATION bin)
