set(CMAKE_SYSTEM_NAME "EMSCRIPTEN")

ADD_DEFINITIONS(-DSAC_DEBUG=1 -DSAC_USE_VBO)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
set(CMAKE_C_COMPILER emcc)
set(CMAKE_CXX_COMPILER emcc)

add_subdirectory(platforms/emscripten)