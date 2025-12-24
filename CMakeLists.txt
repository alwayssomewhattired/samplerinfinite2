cmake_minimum_required(VERSION 3.16)

project(SamplerInfiniteGUI VERSION 0.1 LANGUAGES CXX)



set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets)


set(PROJECT_SOURCES
        main.cpp
        samplerinfinite.cpp
        samplerinfinite.h
        samplerinfinite.ui
        audiodropwidget.cpp
        audiodropwidget.h
        searchablecombobox.cpp
        searchablecombobox.h
        backend/SamplerInfinite.cpp
        include/SamplerInfinite.h
)


    qt_add_executable(SamplerInfiniteGUI
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        audiodropwidget.h audiodropwidget.cpp
        searchablecombobox.h searchablecombobox.cpp
        frequencies.h frequencies.cpp
        backend/FFTProcessor.cpp
        include/FFTProcessor.h
        backend/AudioFileParse.cpp
        include/AudioFileParse.h
        poweroftwospinbox.h poweroftwospinbox.cpp
        demucs/demucsBuild.py
    )


target_link_libraries(SamplerInfiniteGUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(SamplerInfiniteGUI PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS SamplerInfiniteGUI
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(VCPKG_DIR "C:/VCPKG/vcpkg/installed/x64-windows")



target_include_directories(SamplerInfiniteGUI PRIVATE
    ${VCPKG_DIR}/include
)

target_link_directories(SamplerInfiniteGUI PRIVATE
    ${VCPKG_DIR}/lib
)

target_link_libraries(SamplerInfiniteGUI PRIVATE
    ${VCPKG_DIR}/lib/fftw3.lib
    ${VCPKG_DIR}/lib/sndfile.lib
)

set_property(TARGET SamplerInfiniteGUI PROPERTY CXX_STANDARD 20)



target_include_directories(SamplerInfiniteGUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(SamplerInfiniteGUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)


if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SamplerInfiniteGUI)
endif()
