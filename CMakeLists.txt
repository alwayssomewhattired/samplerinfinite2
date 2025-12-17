cmake_minimum_required(VERSION 3.16)

project(SamplerInfiniteGUI VERSION 0.1 LANGUAGES CXX)



set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    )

if (MSVC)
    set_property(TARGET SamplerInfiniteGUI PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    )
endif()

# this prevents qt keywords from stopping libTorch integration
# target_compile_definitions(SamplerInfiniteGUI PRIVATE QT_NO_KEYWORDS)

# compile definitions
add_compile_definitions(USE_CUDA)

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

# set(ONNXRUNTIME_DIR "C:/libs/onnxruntime-win-x64-gpu-1.17.3")
# set(NVIDIATOOLS_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.8")
# set(NVIDIA_DIR "C:/Program Files/NVIDIA/CUDNN/v9.17")
# set(NVIDIA_8_DIR "C:/libs/cudnn-windows-x86_64-8.9.7.29_cuda11-archive")
set(Torch_DIR "C:/libs/libtorch")
set(VCPKG_DIR "C:/VCPKG/vcpkg/installed/x64-windows")

target_include_directories(SamplerInfiniteGUI PRIVATE
    ${Torch_DIR}/include
    ${Torch_DIR}/include/torch/csrc/api/include
    ${VCPKG_DIR}/include
)

target_link_directories(SamplerInfiniteGUI PRIVATE
    ${Torch_DIR}/lib
    ${VCPKG_DIR}/lib
)

target_link_libraries(SamplerInfiniteGUI PRIVATE
    ${Torch_DIR}/lib/torch.lib
    ${Torch_DIR}/lib/torch_cpu.lib
    ${Torch_DIR}/lib/torch_cuda.lib
    ${Torch_DIR}/lib/c10.lib
    ${Torch_DIR}/lib/c10_cuda.lib
    ${VCPKG_DIR}/lib/fftw3.lib
    ${VCPKG_DIR}/lib/sndfile.lib
)

set_property(TARGET SamplerInfiniteGUI PROPERTY CXX_STANDARD 17)

# target_include_directories(SamplerInfiniteGUI PRIVATE ${ONNXRUNTIME_DIR}/include)
# target_include_directories(SamplerInfiniteGUI PRIVATE ${NVIDIATOOLS_DIR}/include)
# target_include_directories(SamplerInfiniteGUI PRIVATE ${NVIDIA_DIR}/include/13.1)
# target_include_directories(SamplerInfiniteGUI PRIVATE ${NVIDIA_8_DIR}/include)





target_include_directories(SamplerInfiniteGUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(SamplerInfiniteGUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# target_link_directories(SamplerInfiniteGUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# target_link_libraries(SamplerInfiniteGUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/fftw3.lib)
# target_link_libraries(SamplerInfiniteGUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/sndfile.lib)



if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SamplerInfiniteGUI)
endif()
