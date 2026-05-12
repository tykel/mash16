add_library(mash16_inspector STATIC inspector.cpp json_server.cpp)
target_link_libraries(mash16_inspector PRIVATE mash16_core)
