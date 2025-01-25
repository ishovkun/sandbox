set (THIS libsandbox)

file(GLOB SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(${THIS} ${SRC})
target_include_directories(${THIS} PRIVATE
  ${logger_include_dirs}
  # ${Seccomp_INCLUDE_DIR}
  BEFORE)
target_link_libraries(${THIS}
  logger
  ${Seccomp_LIBRARIES}
)
