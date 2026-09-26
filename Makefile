# ============================================================
#  Bit24 — Release 构建脚本（双线程并行）
#
#  产物:
#    Bit24_c.exe    <- Bit24.c    (gcc,  C99)
#    Bit24_cpp.exe  <- Bit24.cpp  (g++,  C++11)
#
#  用法:
#    mingw32-make           # 自动双线程并行构建 Release
#    mingw32-make -j2       # 显式双线程，效果相同
#    mingw32-make clean     # 删除构建产物
#
#  Release 产物特性: 全静态链接（零 MinGW DLL 依赖，可直接分发）+ 死代码段回收
# ============================================================

# 使 make 默认以 2 个线程并行构建（两个目标同时编译）
MAKEFLAGS += -j2

CC       := gcc
CXX      := g++

# Release 配置（两个目标共用）
#   -O2 -DNDEBUG -s                        : 优化 + 禁用断言 + 剥离符号
#   -static                                : 全静态链接，消除 MinGW DLL 依赖
#                                            （实测 -static-libgcc -static-libstdc++ 不够，
#                                              仍会静态导入 libwinpthread-1.dll，未装 MinGW 的机器
#                                              会以 0xC0000135 (DLL 未找到) 启动失败）
#   -ffunction-sections -fdata-sections
#   -Wl,--gc-sections                      : 回收未引用代码段（实测 C -2.1% / C++ -1.2%）
RELEASE_FLAGS := -O2 -DNDEBUG -s -static -ffunction-sections -fdata-sections -Wl,--gc-sections

CFLAGS   := -std=c99   -Wall -Wextra $(RELEASE_FLAGS)
CXXFLAGS := -std=c++11 -Wall -Wextra $(RELEASE_FLAGS)

TARGETS := Bit24_c.exe Bit24_cpp.exe

.PHONY: all release clean help

all: release

release: $(TARGETS)

Bit24_c.exe: Bit24.c
	$(CC) $(CFLAGS) -o $@ $<

Bit24_cpp.exe: Bit24.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	powershell -NoProfile -Command "Remove-Item Bit24_c.exe, Bit24_cpp.exe -Force -ErrorAction SilentlyContinue; exit 0"

help:
	@echo Targets:
	@echo   all / release : build $(TARGETS) (Release, 2 parallel jobs)
	@echo   clean         : remove build outputs
