cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(HatchGameEngine VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set up CMake modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

file(GLOB_RECURSE C_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "source/*.c")
file(GLOB_RECURSE CPP_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "source/*.cpp")

set(HATCH_SOURCES ${C_SOURCES} ${CPP_SOURCES})

add_definitions(-DLINUX -DTARGET_NAME=\"${PROJECT_NAME}\" -DUSING_LIBPNG -DUSING_LIBPNG_HEADER=\<libpng16/png.h\>
 -DUSING_LIBJPEG
 -DUSING_FREETYPE
 -DNO_LIBAV
)

add_custom_target(makeheaders ALL
  COMMAND ${CMAKE_SOURCE_DIR}/tools/lmakeheaders source/
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  BYPRODUCTS "${CMAKE_SOURCE_DIR}/makeheaders.bin" "${CMAKE_SOURCE_DIR}/include/Engine"
  COMMENT "Generating header files")

add_executable(${PROJECT_NAME} ${HATCH_SOURCES})
add_dependencies(${PROJECT_NAME} makeheaders)

# Find the libraries
find_package(SDL2 REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(Freetype REQUIRED)

include_directories(
  include
  source
  ${SDL2_INCLUDE_DIRS}
  ${PNG_INCLUDE_DIRS}
  ${JPEG_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
  ${SDL2_LIBRARIES}
  ${SDL2_LIBRARY}
  ${PNG_LIBRARIES}
  ${JPEG_LIBRARIES}
  ${FREETYPE_LIBRARIES}
)