set (THIS libsandbox)

file(GLOB SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(${THIS} ${SRC})
