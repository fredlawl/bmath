cmake_minimum_required(VERSION 3.10)

project(bmath VERSION 1.0.0)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wshadow -Wextra -Wvla -Wmissing-field-initializers")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(LIBRARIES "readline")
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(LIBRARIES ${LIBRARIES} "argp")
else (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(LIBRARIES ${LIBRARIES} "m")
endif()
set(TEST_LIBRARIES ${LIBRARIES})
set(SRC_FILES "")
set(CMAKE_VERBOSE_MAKEFILE ON)

find_path(CRITERION_INCLUDE_DIR NAMES criterion)
find_library(CRITERION_LIBRARY NAMES criterion)
if (CRITERION_LIBRARY)
	include_directories(${CRITERION_INCLUDE_DIR})
	set(TEST_LIBRARIES ${TEST_LIBRARIES} ${CRITERION_LIBRARY})
else(CRITERION_LIBRARY)
    message(WARNING "Unable to locate criterion. Test binary wont build.")
endif(CRITERION_LIBRARY)

find_package(Iconv REQUIRED)
if (Iconv_FOUND)
    include_directories(${Iconv_INCLUDE_DIRS})
    set(LIBRARIES ${LIBRARIES} ${Iconv_LIBRARIES})
else (Iconv_FOUND)
    message(ERROR "Unable to find iconv")
endif (Iconv_FOUND)

set(SRC_FILES ${SRC_FILES} src/parser.c)

add_compile_definitions(VERSION="${PROJECT_VERSION}")

add_executable(${PROJECT_NAME} src/bmath.c ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

if (CRITERION_LIBRARY)
	set(SRC_FILES ${SRC_FILES} test/conversions.c test/parser.c)
	add_executable(${PROJECT_NAME}_test ${SRC_FILES})
	target_link_libraries(${PROJECT_NAME}_test ${TEST_LIBRARIES})
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
