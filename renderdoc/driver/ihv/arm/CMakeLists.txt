set(sources 
    arm_gl_counters.cpp 
    arm_gl_counters.h) 
 
set(include_dirs ${RDOC_INCLUDES}) 
 
add_library(rdoc_arm OBJECT ${sources}) 
target_compile_definitions(rdoc_arm ${RDOC_DEFINITIONS}) 
target_include_directories(rdoc_arm ${include_dirs})
