set(core_SRCS audio.cpp cpu.cpp cpu_ops.cpp cpu_rec.cpp cpu_rec_invalidate.cpp cpu_rec_ops.cpp gpu.cpp)
add_library(mash16_core STATIC ${core_SRCS})
