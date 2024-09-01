cmake_minimum_required(VERSION 3.5)

project(Senti)

set(ZLIB_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/External/Lib")
set(ZLIB_LIBRARY "zlibstat.lib")
link_directories(${ZLIB_LIBRARY_DIR})

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")


# add header path
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.91.0
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.91.0/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/External/Include
)


# set common source
file(GLOB SRC
        ./imgui-1.91.0/*.h
        ./imgui-1.91.0/*.cpp
)

# set specific source and other option for platform

    file(GLOB PLATFORM_SRC
            ./imgui-1.91.0/backends/imgui_impl_win32.*
            ./imgui-1.91.0/backends/imgui_impl_dx12.*
            ./src/win/main.cpp
    )

# generate binary
add_executable(${PROJECT_NAME} ${SRC} ${PLATFORM_SRC}
        src/win/UI.cpp
        src/win/UI.h
        src/win/Lexend.h
        src/win/Process.cpp
        src/win/Process.h
        src/win/CRC32.cpp
        src/win/CRC32.h
        src/win/Logger.cpp
        src/win/Logger.h
        src/win/Menu.cpp
        src/win/Menu.h)

# link lib, should install glfw first or prebuild lib and embed in project
    target_link_libraries(${PROJECT_NAME}
            d3d12.lib
            d3dcompiler.lib
            dxgi.lib
            ${ZLIB_LIBRARY}
    )