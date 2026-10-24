# file-format: 1.0.0
cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

include(cmake/target.cmake)
include(cmake/${TOOLCHAIN_FILE})

project(C5_Car)
add_executable(C5_Car)
include(cmake/files.cmake)
include(cmake/flags.cmake)
include(cmake/components.cmake)

# 应用模块
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  app/debug_uart.c
  app/encoder.c
  app/motor_driver.c
  app/speed_pid.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  app
)
