add_library(v4l2_property SHARED
    propertybrowser.cpp
)

target_link_libraries(v4l2_property PUBLIC
    v4l2_core
    Qt6::Widgets
)

target_include_directories(v4l2_property PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/v4l2-test-gui/core
)

set_target_properties(v4l2_property PROPERTIES
    AUTOMOC ON
)
