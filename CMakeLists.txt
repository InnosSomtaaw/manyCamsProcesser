cmake_minimum_required(VERSION 3.5)

project(camsHotSpot VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "-fpermissive")
#set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_SEARCH_MODULE(PKG_OPENCV REQUIRED opencv4)
PKG_SEARCH_MODULE(PKG_AVCODEC REQUIRED libavcodec)
PKG_SEARCH_MODULE(PKG_AVFORMAT REQUIRED libavformat)
PKG_SEARCH_MODULE(PKG_AVUTIL REQUIRED libavutil)
PKG_SEARCH_MODULE(PKG_SWRESAMPLE REQUIRED libswresample)
PKG_SEARCH_MODULE(PKG_SWSCALE REQUIRED libswscale)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        pictureview.h
        pictureview.cpp
        ImageProcess/image_processing.h
        ImageProcess/image_processing.cpp
        ImageProcess/trtinferencer.h
        ImageProcess/trtinferencer.cpp
        ImageProcess/yolopostprocess.h
        ImageProcess/yolopostprocess.cpp
        ImageProcess/integrateddisplay.h
        ImageProcess/integrateddisplay.cpp
        Camera/general_camera.h
        Camera/general_camera.cpp
        Camera/hwdecoder.h
        Camera/hwdecoder.cpp
        Camera/videoplayer.h
        Camera/videoplayer.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(hikCamTRT
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET 4camsCapture APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(camsHotSpot SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(camsHotSpot
            ${PROJECT_SOURCES}
        )
    endif()
endif()

include_directories(
    ${PKG_OPENCV_INCLUDE_DIRS}
    ${PKG_AVCODEC_INCLUDE_DIRS}
    ${PKG_AVFORMAT_INCLUDE_DIRS}
    ${PKG_AVUTIL_INCLUDE_DIRS}
    ${PKG_SWRESAMPLE_INCLUDE_DIRS}
    ${PKG_SWSCALE_INCLUDE_DIRS}
    /home/TensorRT-8.6.1.6/include
    /usr/local/cuda-12.1/targets/x86_64-linux/include)

target_link_libraries(camsHotSpot PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
    ${PKG_OPENCV_LDFLAGS}
    ${PKG_AVCODEC_LDFLAGS}
    ${PKG_AVFORMAT_LDFLAGS}
    ${PKG_AVUTIL_LDFLAGS}
    ${PKG_SWRESAMPLE_LDFLAGS}
    ${PKG_SWSCALE_LDFLAGS}
    -lyaml-cpp
    -L/home/TensorRT-8.6.1.6/lib/ -lnvonnxparser -lnvinfer
    -L/usr/local/cuda/lib64/ -lcudart)

set_target_properties(camsHotSpot PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(hikCamTRT)
endif()
