cmake_minimum_required(VERSION 3.16)

project(v4l2-test-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON) 

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(OpenCV REQUIRED)

set(UI
    mainwindow.ui
    mainwindow.qrc
)

set(HEADERS
    mainwindow.hpp
    mainwindow.ui
    mainwindow.qrc
)

set(SOURCES
    main.cpp
    mainwindow.cpp
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

target_link_libraries(${PROJECT_NAME} PRIVATE
    common
    v4l2_property
    Qt6::Widgets 
    Qt6::Network
    ${OpenCV_LIBS})

add_subdirectory(core)
add_subdirectory(property)
add_subdirectory(image)
add_subdirectory(plugins)

# External Plugins (optional)
# All subdirectories with CMakeLists.txt are automatically included
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins-external")
    file(GLOB EXTERNAL_PLUGIN_DIRS 
         LIST_DIRECTORIES true 
         "${CMAKE_CURRENT_SOURCE_DIR}/plugins-external/*")
    
    foreach(PLUGIN_DIR ${EXTERNAL_PLUGIN_DIRS})
        if(IS_DIRECTORY ${PLUGIN_DIR} AND EXISTS "${PLUGIN_DIR}/CMakeLists.txt")
            get_filename_component(PLUGIN_NAME ${PLUGIN_DIR} NAME)
            message(STATUS "Including external plugin: ${PLUGIN_NAME}")
            add_subdirectory(plugins-external/${PLUGIN_NAME})
        endif()
    endforeach()
endif()