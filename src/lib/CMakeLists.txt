set (THIS libsandbox)

file(GLOB SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(${THIS} ${SRC})
target_include_directories(${THIS} PRIVATE
  BEFORE)
target_link_libraries(${THIS}
  ${Seccomp_LIBRARIES}
)
