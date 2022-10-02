cmake_minimum_required (VERSION 3.21)
project(mt6502)

set(CMAKE_CXX_STANDARD   23)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/Modules)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT error)

if (IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# See Compilers
# https://github.com/cpp-best-practices/cppbestpractices/blob/master/02-Use_the_Tools_Available.md

message(DEBUG "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    STRING(JOIN " "
        CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS}"
        "-Wall"                # warn almost everything
        "-Wextra"              # warn even more
        "-Wshadow"             # warn when name shadowing
        "-Wnon-virtual-dtor"   # warn on class with virtual methods has no virtual destructor
        "-pedantic"            # warn on language extensions
        "-Wold-style-cast"     # warn on c style casting
        "-Wcast-align"         # warn on potential performance problem casts
        "-Wunused"             # warn on anything not used
        "-Woverloaded-virtual" # warn if virtual function was overloaded instead of overriden
        "-Wpedantic"           # warn if non-standard c++ is used
        "-Wconversion"         # warn on type conversion that loses data
        "-Wsign-conversion"    # warn on sign conversion
        "-Wmisleading-indentation" # warn if indentation implies blocks where blocks do not exist
        "-Wnull-dereference"   # warn if a null dereference is detected
        "-Wdouble-promotion"   # warn if float is implicit promoted to double
        "-Wformat=2"           # warn on security issues in print formatted
        "-Weverything"         # enable every possible warning
        "-Weffc++"             # enforce effective c++ programming rules
        "-Wno-c++98-compat"    # ignore c++98 compatibility issues
        "-Werror"              # treat warn as errors
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        STRING(JOIN " "
            CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS}"
            "-Wuseless-cast"    # warn on perform cast to same type
            "-Wduplicated-cond" # warn on if-else that has duplicated conditions
        )
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        STRING(JOIN " "
            "-fsanitize=address"
            # "-fsanitize=memory"
            # "-fsanitize=thread"
            "-fsanitize=undefined"
             CMAKE_CXX_FLAGS_DEBUG)

        if (CMAKE_SYSTEM_NAME MATCHES "Linux")
            if (IPO_SUPPORTED)
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=cfi")
            endif()
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=dataflow")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=safe-stack")
        endif()
    endif()
endif()
message(DEBUG "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(DEBUG "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")

add_library(${PROJECT_NAME} src/mos6502/bus.cpp src/mos6502/cpu.cpp)
target_include_directories(${PROJECT_NAME}  PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

add_executable(${PROJECT_NAME}_bench bench/mos6502_bench.cpp)
target_link_libraries(${PROJECT_NAME}_bench PRIVATE ${PROJECT_NAME} )

find_package(Catch2 CONFIG REQUIRED)

add_executable(${PROJECT_NAME}_test test/mos6502_test.cpp)
target_link_libraries(${PROJECT_NAME}_test PRIVATE ${PROJECT_NAME}  Catch2::Catch2WithMain)