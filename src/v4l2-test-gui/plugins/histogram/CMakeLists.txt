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
if(APPLE)
    set_target_properties(histogram_plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        AUTOMOC ON
        BUILD_RPATH "${CMAKE_BINARY_DIR}/src/v4l2-test-gui/core"
    )
    
    # Copy plugin into app bundle after build
    add_custom_command(TARGET histogram_plugin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:v4l2-test-gui>/Contents/PlugIns"
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:histogram_plugin>"
            "$<TARGET_BUNDLE_DIR:v4l2-test-gui>/Contents/PlugIns/"
        # Fix RPATH to use relative paths within app bundle
        COMMAND install_name_tool -delete_rpath "${CMAKE_BINARY_DIR}/src/v4l2-test-gui/core"
            "$<TARGET_BUNDLE_DIR:v4l2-test-gui>/Contents/PlugIns/$<TARGET_FILE_NAME:histogram_plugin>" || true
        COMMAND install_name_tool -add_rpath "@loader_path/../../Frameworks"
            "$<TARGET_BUNDLE_DIR:v4l2-test-gui>/Contents/PlugIns/$<TARGET_FILE_NAME:histogram_plugin>" || true
        COMMAND install_name_tool -add_rpath "@executable_path/../Frameworks"
            "$<TARGET_BUNDLE_DIR:v4l2-test-gui>/Contents/PlugIns/$<TARGET_FILE_NAME:histogram_plugin>" || true
        COMMENT "Copying histogram plugin to app bundle and fixing RPATH"
    )
else()
    set_target_properties(histogram_plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        AUTOMOC ON
        BUILD_RPATH "${CMAKE_BINARY_DIR}/src/v4l2-test-gui/core"
        INSTALL_RPATH "$ORIGIN/../lib"
    )
endif()

# Install to plugins directory
install(TARGETS histogram_plugin
    LIBRARY DESTINATION plugins
)