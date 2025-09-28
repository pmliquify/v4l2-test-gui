add_library(v4l2_core SHARED
    roi.cpp
    function.cpp
    functionplugin.cpp
    projectmanager.cpp
)

target_link_libraries(v4l2_core PUBLIC
    Qt6::Core
    Qt6::Gui
    opencv_core
)

target_include_directories(v4l2_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/v4l2-test-gui/property
)

set_target_properties(v4l2_core PROPERTIES
    AUTOMOC ON
)

# Link the main application against the core library
target_link_libraries(${PROJECT_NAME} PRIVATE v4l2_core)