cmake_minimum_required(VERSION 3.22)
project(assignment C)

set(CMAKE_C_STANDARD 11)

add_library(model OBJECT
        defs.h
        interface.h
        model.c
        model.h
)

add_executable(interactive
        interface.c
)

add_executable(testrunner
        testrunner.c
        testrunner.h
        tests.c
        tests.h
)
target_link_libraries(testrunner model)

if(${MINGW})
        cmake_path(GET CMAKE_C_COMPILER PARENT_PATH BIN_DIR)
        cmake_path(GET BIN_DIR PARENT_PATH MINGW_DIR)
        message(MINGW_DIR=${MINGW_DIR})
        target_include_directories(interactive PUBLIC ${MINGW_DIR}/opt/include ${MINGW_DIR}/opt/include/ncursesw)
        target_link_directories(interactive PUBLIC ${MINGW_DIR}/opt/lib)
        target_link_libraries(interactive ncursesw model)
else()
        find_package(Curses REQUIRED)
        target_include_directories(interactive PUBLIC ${CURSES_INCLUDE_DIR})
        target_link_libraries(interactive ${CURSES_LIBRARIES} model)
endif()

