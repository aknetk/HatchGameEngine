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

IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_definitions(-DLINUX -DTARGET_NAME=\"${PROJECT_NAME}\"
   -DUSING_FREETYPE
   -DNO_LIBAV
  )
else()
  # Cross compiling a Windows executable
  add_definitions(-DWIN32 -DTARGET_NAME=\"${PROJECT_NAME}\"
   -DNO_LIBAV
   -DGLEW_STATIC
   -D_WINDOWS
  )
endif()

add_custom_target(makeheaders ALL
  COMMAND ${CMAKE_SOURCE_DIR}/tools/lmakeheaders source/
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  BYPRODUCTS "${CMAKE_SOURCE_DIR}/makeheaders.bin" "${CMAKE_SOURCE_DIR}/include/Engine"
  COMMENT "Generating header files")

add_executable(${PROJECT_NAME} ${HATCH_SOURCES})
add_dependencies(${PROJECT_NAME} makeheaders)

# Find the libraries
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  find_package(SDL2 REQUIRED)
  find_package(Freetype REQUIRED)
else()
  # Set the paths to the MinGW libraries
  set(SDL2_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/meta/win/lib/mingw/${TOOLCHAIN_PREFIX}/libSDL2.dll.a;${CMAKE_CURRENT_LIST_DIR}/meta/win/lib/mingw/${TOOLCHAIN_PREFIX}/libSDL2main.a")
  set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/meta/win/include/SDL2/")
endif()

# Add needed directories
include_directories(
  include
  source
  ${SDL2_INCLUDE_DIRS}
)

# Include libraries
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # Add all of the Windows include files
  include_directories(
    meta/win/include
  )
else()
  include_directories(
    ${FREETYPE_INCLUDE_DIRS}
  )
endif()

# Add SDL2 libraries
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

# Add other libraries
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # All that fun stuff
  # MinGW libraries are linked statically
  target_link_libraries(${PROJECT_NAME}
    -static
    -lmingw32 -mwindows
    -lws2_32)
  add_definitions(-DSDL_MAIN_HANDLED)
else()
  target_link_libraries(${PROJECT_NAME}
    ${FREETYPE_LIBRARIES}
  )
endif()
