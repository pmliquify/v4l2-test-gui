
set(SOURCES
        image.cpp
)

set(HEADERS
        image.hpp
        sockettypes.hpp
)

add_library(common STATIC ${SOURCES} ${HEADERS})
target_include_directories(common PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR})