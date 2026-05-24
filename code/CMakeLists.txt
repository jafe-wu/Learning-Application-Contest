cmake_minimum_required(VERSION 3.15)
project(cigarette_pallet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- EasyX paths (auto-detect from VSINSTALLDIR env, or override manually) ----
if(DEFINED ENV{VSINSTALLDIR})
    set(_VS_DIR "$ENV{VSINSTALLDIR}")
elseif(EXISTS "E:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/VS")
    set(_VS_DIR "E:/Program Files/Microsoft Visual Studio/2022/Community")
elseif(EXISTS "D:/Visual Studio/VC/Auxiliary/VS")
    # Custom install path (common for this project)
    set(_VS_DIR "D:/Visual Studio")
else()
    # Default VS Community path
    set(_VS_DIR "C:/Program Files/Microsoft Visual Studio/2022/Community")
endif()
set(EASYX_INCLUDE "${_VS_DIR}/VC/Auxiliary/VS/include")
set(EASYX_LIB_DIR "${_VS_DIR}/VC/Auxiliary/VS/lib/x64")

add_executable(main
    main.cpp
    PalletAlgorithm.cpp
    PalletizerController.cpp
    DimDB.cpp
    OrderLoader.cpp
    Visualizer.cpp
)

# 仅 MSVC 下使用 EasyX (MinGW 的 graphics.h 是 EGE，不兼容)
if(MSVC)
    target_compile_options(main PRIVATE /EHsc /source-charset:utf-8 /execution-charset:gbk)
    target_compile_definitions(main PRIVATE UNICODE _UNICODE)
    target_include_directories(main PRIVATE "${EASYX_INCLUDE}")
    target_link_directories(main PRIVATE "${EASYX_LIB_DIR}")
    target_link_libraries(main PRIVATE user32 gdi32 shell32 EasyXw)
else()
    message(FATAL_ERROR
        "This project requires MSVC + EasyX. "
        "Please switch CLion toolchain to Visual Studio.")
endif()
