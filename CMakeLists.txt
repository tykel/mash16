cmake_minimum_required(VERSION 3.15)

project(mash16 VERSION 2.0 LANGUAGES CXX)

enable_testing()

option(ENABLE_PROFILING "Enable run-time profiling output for gprof" OFF)

option(BUILD_INSPECTOR "Build inspector introspection module" OFF)

set(MASH16_USER_BIN "$ENV{HOME}/bin" CACHE PATH "User bin directory for local mash16 installs")
set(MASH16_DEBUG_BUILD_DIR "${PROJECT_SOURCE_DIR}/build-debug" CACHE PATH "Debug build directory for local mash16 installs")

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
find_package(OpenGL REQUIRED)
find_package(Git REQUIRED)

set(CMAKE_CXX_STANDARD 20)
if(BUILD_INSPECTOR)
    add_compile_definitions(BUILD_INSPECTOR)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
   add_compile_options(-O0 -g -ggdb -fno-omit-frame-pointer -fsanitize=address)
   add_link_options(-fsanitize=address)
else()
   add_compile_options(-O3)
endif()

if(ENABLE_PROFILING)
    add_compile_options(-pg -fno-inline-functions)
    add_link_options(-pg)
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
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/core")
include_directories("${PROJECT_SOURCE_DIR}/header")
add_subdirectory(src)

add_executable(mash16 src/options.cpp src/strings.cpp src/main.cpp)
add_executable(cpu_test src/options.cpp src/strings.cpp src/test/cpu_test.cpp)
add_executable(jit_sanity_test src/options.cpp src/strings.cpp src/test/jit_sanity_test.cpp)
add_executable(opcode_behavior_test src/options.cpp src/strings.cpp src/test/opcode_behavior_test.cpp)
add_test(NAME jit_sanity_test COMMAND jit_sanity_test)
add_test(NAME opcode_behavior_test COMMAND opcode_behavior_test)
set_tests_properties(jit_sanity_test PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")
set_tests_properties(opcode_behavior_test PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")

if(BUILD_INSPECTOR)
    set_target_properties(mash16 PROPERTIES EXPORT_NAME "mash16-debug")
    add_executable(mash16-inspect src/inspector/mash16_inspect.cpp)
    add_executable(inspector_server_test src/options.cpp src/strings.cpp src/test/inspector_server_test.cpp)
    target_link_libraries(inspector_server_test PRIVATE mash16_header mash16_core mash16_inspector SDL2::SDL2 OpenGL::GL imgui_impl_sdl2 imgui_impl_opengl2)
    add_executable(inspector_subscribe_test src/options.cpp src/strings.cpp src/test/inspector_subscribe_test.cpp)
    target_link_libraries(inspector_subscribe_test PRIVATE mash16_header mash16_core mash16_inspector SDL2::SDL2 OpenGL::GL imgui_impl_sdl2 imgui_impl_opengl2)
    add_test(NAME inspector_server_test COMMAND inspector_server_test)
    add_test(NAME inspector_subscribe_test COMMAND inspector_subscribe_test)
    set_tests_properties(inspector_server_test inspector_subscribe_test PROPERTIES RUN_SERIAL TRUE)
endif()

# Link SDL2
if(TARGET SDL2::SDL2main)
    target_link_libraries(mash16 PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(mash16 PRIVATE mash16_header mash16_core SDL2::SDL2 OpenGL::GL imgui_impl_sdl2 imgui_impl_opengl2)
target_link_libraries(cpu_test PRIVATE mash16_header mash16_core SDL2::SDL2 OpenGL::GL imgui_impl_sdl2 imgui_impl_opengl2)
target_link_libraries(jit_sanity_test PRIVATE mash16_header mash16_core SDL2::SDL2 OpenGL::GL imgui_impl_sdl2 imgui_impl_opengl2)
target_link_libraries(opcode_behavior_test PRIVATE mash16_header mash16_core SDL2::SDL2 OpenGL::GL imgui_impl_sdl2 imgui_impl_opengl2)
if(BUILD_INSPECTOR)
    target_link_libraries(mash16 PRIVATE mash16_inspector)
endif()

install(TARGETS mash16 DESTINATION bin)

if(BUILD_INSPECTOR)
    install(TARGETS mash16-inspect DESTINATION bin)
    add_executable(inspector_test src/options.cpp src/strings.cpp src/test/inspector_test.cpp)
    target_link_libraries(inspector_test PRIVATE mash16_header mash16_core mash16_inspector SDL2::SDL2 OpenGL::GL imgui_impl_sdl2 imgui_impl_opengl2)
    add_test(NAME inspector_test COMMAND inspector_test)
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_target(install-user
        COMMAND /bin/sh "${PROJECT_SOURCE_DIR}/scripts/install-user.sh"
                "$<TARGET_FILE:mash16>"
                "${MASH16_DEBUG_BUILD_DIR}/mash16"
                "${MASH16_DEBUG_BUILD_DIR}/mash16-inspect"
                "${MASH16_USER_BIN}"
        DEPENDS mash16
        COMMENT "Installing mash16 user binaries to ${MASH16_USER_BIN}"
        VERBATIM
    )
endif()
