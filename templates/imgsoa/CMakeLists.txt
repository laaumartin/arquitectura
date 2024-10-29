add_library(imgsoa
        files.cpp)
target_link_libraries(imgsoa PUBLIC common PRIVATE Microsoft.GSL::GSL)