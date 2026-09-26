# ============================================================
#  Bit24 — 跨平台构建（CMake >= 3.15）
#
#  产物:
#    Bit24_c    <- Bit24.c    (C99)
#    Bit24_cpp  <- Bit24.cpp  (C++11)
#    （Windows 上自动带 .exe 后缀，Linux/macOS 上无后缀）
#
#  用法:
#    cmake -S . -B build                 # 配置（默认 Release）
#    cmake --build build --parallel 2    # 双线程并行构建
#    cmake --build build                 # 构建工具自行决定并行度
#
#  显式指定构建类型:
#    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
# ============================================================

cmake_minimum_required(VERSION 3.15)

project(Bit24
    VERSION 1.0
    DESCRIPTION "24-game solver with bitwise operators"
    LANGUAGES C CXX
)

# ---- 默认构建类型: Release（未显式指定时） ----
# Release 在 GCC/Clang 下是 -O3 -DNDEBUG，在 MSVC 下是 /O2 /DNDEBUG，
# 由 CMake 按工具链自动选择，无需手写平台相关 flag。
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Release/Debug/...)" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# ---- 语言标准（跨编译器统一，MSVC/GCC/Clang 通用） ----
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- 警告：按编译器选择，不硬编码 GCC 专属 flag ----
if(MSVC)
    add_compile_options(/W4)
else() # GCC / Clang / AppleClang / MinGW
    add_compile_options(-Wall -Wextra)
endif()

# ---- 目标 ----
add_executable(Bit24_c   Bit24.c)
add_executable(Bit24_cpp Bit24.cpp)

# ---- 体积优化: 死代码段回收 + 剥离符号 ----
if(NOT MSVC AND NOT APPLE)
    foreach(t Bit24_c Bit24_cpp)
        target_compile_options(${t} PRIVATE -ffunction-sections -fdata-sections)
        target_link_options(${t} PRIVATE -Wl,--gc-sections $<$<CONFIG:Release>:-s>)
    endforeach()
endif()

# ---- Release: MinGW 下全静态链接，得到零 MinGW 运行时依赖的可分发产物 ----
if(MINGW)
    foreach(t Bit24_c Bit24_cpp)
        target_link_options(${t} PRIVATE $<$<CONFIG:Release>:-static>)
    endforeach()
endif()
