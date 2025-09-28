target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/v4l2-test-gui
)

target_sources(${PROJECT_NAME} PRIVATE
    socketserver.cpp
    convert.cpp
    imagewidget.cpp
)