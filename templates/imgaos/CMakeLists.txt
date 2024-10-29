add_library(imgaos
        files.cpp.hpp)
target_link_libraries(imgaos PUBLIC common PRIVATE Microsoft.GSL::GSL)