cmake_minimum_required(VERSION 3.29)
project(imtool-2024 VERSION 1.0 LANGUAGES CXX)

# Print processor and compiler
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

# Set standard version and requirements
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler specific options
add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wextra -Wconversion -Wsign-conversion)

# Support FetchContent functionality
include(FetchContent)

# Unit tests with google test
#Fetch googletest
FetchContent_Declare(googletest
        GIT_REPOSITORY "https://github.com/google/googletest"
        GIT_TAG "v1.15.2"
        GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(googletest)

# Enable GSL
FetchContent_Declare(GSL
        GIT_REPOSITORY "https://github.com/microsoft/GSL"
        GIT_TAG "v4.0.0"
        GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(GSL)

# Enable clang-tidy
set(CMAKE_CXX_CLANG_TIDY clang-tidy-19;
        -format-style=file;
        -header-filter=.;)


# Add include directory
include_directories(PUBLIC .)

# Add library subdirectory
add_subdirectory(imgaos)
add_subdirectory(imgsoa)
add_subdirectory(common)

# Add unit tests
if (IMG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(utest-img)
    add_subdirectory(utest-common)
endif ()

# Add executable
add_subdirectory(imtool-aos)
add_subdirectory(imtool-soa)