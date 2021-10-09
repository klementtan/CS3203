# CMakeLists.txt

file(GLOB_RECURSE srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE headers "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE includes "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE externals "${CMAKE_CURRENT_SOURCE_DIR}/external/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/external/*.hpp")

add_library(spa ${srcs} ${headers} ${includes} ${externals} "src/pkb/cfg.cpp")

if (WIN32)
	set(WARNING_FLAGS "/permissive- /W3 /w14265 /w14263 /w14548")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")
else()
	set(WARNING_FLAGS "-Wall -Wextra -Wno-old-style-cast -Wno-unused-parameter -Werror=return-type")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")
endif()

# this makes the headers accessible for other projects which uses spa lib
target_include_directories(spa PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/external
)

