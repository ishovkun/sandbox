set (THIS libsandbox)

file(GLOB SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(${THIS} ${SRC})

set(DEPS "")
set(INCLUDES "")

if (Seccomp_FOUND)
  list(APPEND DEPS ${Seccomp_LIBRARIES})
  list(APPEND INCLUDES ${Seccomp_INCLUDE_DIRS})
endif()


target_include_directories(${THIS} PRIVATE ${INCLUDES} BEFORE)
target_link_libraries(${THIS} ${DEPS})
if (Seccomp_FOUND)
  target_compile_definitions(${THIS} PRIVATE -DUSE_SECCOMP)
endif()
