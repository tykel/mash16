cmake_minimum_required(VERSION 3.15)

project(mash16 VERSION 2.0 LANGUAGES C)

add_compile_options(-mssse3)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
find_package(Git QUIET)

if(Git_FOUND AND EXISTS ${PROJECT_SOURCE_DIR}/.git)
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

add_executable(mash16 src/options.c src/strings.c src/main.c)
add_executable(cpu_test src/options.c src/strings.c src/cpu_test.c)

# Link SDL2
if(TARGET SDL2::SDL2main)
    target_link_libraries(mash16 PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(mash16 PRIVATE SDL2::SDL2)
target_link_libraries(cpu_test PRIVATE SDL2::SDL2)

target_link_libraries(mash16 PRIVATE mash16_header mash16_core)
target_link_libraries(cpu_test PRIVATE mash16_header mash16_core)

install(TARGETS mash16 DESTINATION bin)
