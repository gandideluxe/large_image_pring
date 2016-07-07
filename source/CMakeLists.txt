file(GLOB_RECURSE FRAMEWORK_SHADER_VERTEX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.vert)
file(GLOB_RECURSE FRAMEWORK_SHADER_FRAGME RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.frag)

add_executable(LIS_experiment LIS_experiment_main.cpp
              ${FRAMEWORK_SHADER_VERTEX}
              ${FRAMEWORK_SHADER_FRAGME})

target_link_libraries(LIS_experiment ${FRAMEWORK_NAME} ${BINARY_FILES})
add_dependencies(LIS_experiment glfw ${FRAMEWORK_NAME} ${COPY_BINARY})

install(TARGETS LIS_experiment DESTINATION .)
