add_library(histogram_plugin MODULE
    histogram.cpp
)

target_link_libraries(histogram_plugin PRIVATE
    v4l2_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    opencv_core
    opencv_imgproc
)

target_include_directories(histogram_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set output directory for plugins
set_target_properties(histogram_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    AUTOMOC ON
    BUILD_RPATH "${CMAKE_BINARY_DIR}/src/v4l2-test-gui/core"
    INSTALL_RPATH "@loader_path/../Frameworks"
)

# Install to plugins directory
install(TARGETS histogram_plugin
    LIBRARY DESTINATION plugins
)