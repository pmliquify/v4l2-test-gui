cmake_minimum_required(VERSION 3.16)

project(v4l2-test-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON) 

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets 
    Network
    Charts
)
find_package(OpenCV REQUIRED)

set(UI
    mainwindow.ui
    mainwindow.qrc
)

set(SOURCES
    main.cpp
    mainwindow.cpp
    imagewidget.cpp
    convert.cpp
    socketserver.cpp
    roi.cpp
    projectmanager.cpp
    histogram.cpp
    function.cpp
)

set(HEADERS
    mainwindow.hpp
    mainwindow.ui
    mainwindow.qrc
    imagewidget.hpp
    convert.hpp
    socketserver.hpp
    roi.hpp
    projectmanager.hpp
    histogram.hpp
    function.hpp
)

if (APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${UI} ${SOURCES} ${HEADERS})    
    target_sources(${PROJECT_NAME} PRIVATE icons/app.icns)
    set_source_files_properties(icons/app.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        MACOSX_BUNDLE_ICON_FILE app.icns)
else()
    add_executable(${PROJECT_NAME} ${UI} ${SOURCES} ${HEADERS})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR}/..)

target_link_libraries(${PROJECT_NAME} 
    common
    Qt6::Widgets 
    Qt6::Network
    Qt6::Charts
    ${OpenCV_LIBS})