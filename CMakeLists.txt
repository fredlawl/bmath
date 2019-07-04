cmake_minimum_required(VERSION 3.12)
project(bmath)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wshadow -Wextra -Wvla -Wmissing-field-initializers")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARIES "argp")
set(TEST_LIBRARIES "${LIBRARIES}")
set(SRC_FILES "")

include_directories("/usr/local/include/")
link_directories("/usr/local/lib")

find_path(CRITERION_INCLUDE_DIR NAMES criterion)
find_library(CRITERION_LIBRARY NAMES criterion REQUIRED)
if (CRITERION_LIBRARY)
	include_directories(${CRITERION_INCLUDE_DIR})
	set(TEST_LIBRARIES ${TEST_LIBRARIES} ${CRITERION_LIBRARY})
else(CRITERION_LIBRARY)
	message(FATAL_ERROR "Unable to locate criterion. Test binary wont build.")
endif(CRITERION_LIBRARY)

set(SRC_FILES ${SRC_FILES} src/parser.c)

add_executable(${PROJECT_NAME} src/bmath.c ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

if (CRITERION_LIBRARY)
	set(SRC_FILES ${SRC_FILES} test/conversions.c test/parser.c)
	add_executable(${PROJECT_NAME}_test ${SRC_FILES})
	target_link_libraries(${PROJECT_NAME}_test ${TEST_LIBRARIES})
endif()