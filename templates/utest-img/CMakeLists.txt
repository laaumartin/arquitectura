add_executable(utest-img
        file.cpp
        otherfile.cpp)

target_link_libraries(utest-img PRIVATE imgaos imgsoa GTest::gtest_main Microsoft.GSL::GSL)