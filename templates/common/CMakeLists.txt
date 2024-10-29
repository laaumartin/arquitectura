add_library(common
        progargs.hpp progargs.cpp
        otherfile.cpp)
target_link_libraries(common PRIVATE Microsoft.GSL::GSL)


