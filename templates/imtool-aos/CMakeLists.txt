add_executable(imtool-aos main.cpp)
target_link_libraries(imtool-aos PRIVATE imgaos Microsoft.GSL::GSL)