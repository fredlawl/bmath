cmake_minimum_required(VERSION 3.10)

project(bmath VERSION 1.0.3 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS_RELEASE "-Wl,--build-id -O3 -DNDEBUG")

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wshadow -Wextra -Wvla -Wmissing-field-initializers -fno-omit-frame-pointer")
#set(CMAKE_BINARY_DIR "${PROJECT_SOURCE_DIR}/bin")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/release")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/debug")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h"
)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    include_directories("/usr/include" "/usr/local/include")
    link_directories("/usr/lib" "/usr/local/lib")
    set(LIBRARIES ${LIBRARIES} "argp")
    set(LIBRARIES "readline")
else (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(LIBRARIES ${LIBRARIES} "-lm")
    set(LIBRARIES ${LIBRARIES} "-l:libreadline.a -lcurses")
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
  #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/test)
  #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/test)
  set(SRC_FILES ${SRC_FILES} test/parser.c)
  add_executable(${PROJECT_NAME}_test ${SRC_FILES})
  target_link_libraries(${PROJECT_NAME}_test ${TEST_LIBRARIES})
  install(TARGETS ${PROJECT_NAME}_test RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/local/bin)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/local/bin)
