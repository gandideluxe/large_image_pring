file(GLOB_RECURSE FRAMEWORK_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
file(GLOB_RECURSE FRAMEWORK_INLINE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.inl)
file(GLOB_RECURSE FRAMEWORK_HEADER RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.hpp *.h)
file(GLOB_RECURSE FRAMEWORK_SHADER_VERTEX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.vert)
file(GLOB_RECURSE FRAMEWORK_SHADER_FRAGME RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.frag)
file(GLOB_RECURSE FRAMEWORK_SHADER_COMP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cu)

source_group("Inline Files" FILES ${FRAMEWORK_INLINE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(${FRAMEWORK_NAME} STATIC glew.c
  ${FRAMEWORK_SOURCE}
  ${FRAMEWORK_INLINE}
  ${FRAMEWORK_HEADER}
  ${FRAMEWORK_SHADER_VERTEX}
  ${FRAMEWORK_SHADER_FRAGME}
  ${FRAMEWORK_SHADER_COMP})
