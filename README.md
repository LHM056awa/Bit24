# 二十四点求解器 (支持位运算)

一个用 C++ 与 C 编写的控制台程序（两种实现完全等价），对输入的四个整数穷举所有可能的运算组合（包含四则运算与位运算），找出一个等于 **24** 的表达式，并以 `/24` 前缀输出。

## 功能特点

- **支持运算符**：
  - 四则运算：`+` `-` `*` `/`
  - 位运算：`&` `|` `^` `<<` `>>`
- **数学除法**：使用浮点数计算，保证 `(3/2)*16` 这类表达式得到精确 24（不再是整数除法）。
- **位运算仅在操作数为整数时进行**，并限制在无符号64位范围内，安全可靠。
- **五种括号结构**：自动组合，确保找到正确的运算顺序。

## 编译

项目包含两个等价的实现，任选其一编译：

| 源文件 | 语言标准 | 编译器 |
| --- | --- | --- |
| `Bit24.c` | C99 | gcc / clang |
| `Bit24.cpp` | C++11 | g++ / clang++ / MSVC |

### 方式一：Makefile（Windows + MinGW，默认双线程并行）

```bash
mingw32-make          # 构建 Bit24_c.exe 与 Bit24_cpp.exe（Release，2 线程并行）
mingw32-make clean    # 删除构建产物
```

### 方式二：CMake（跨平台，需 CMake >= 3.15）

```bash
cmake -S . -B build                 # 配置（默认 Release）
cmake --build build --parallel 2    # 双线程并行构建
cmake --build build --target clean  # 清理
```

### 方式三：手动编译

#### C 版本

```bash
gcc -std=c99 -Wall -Wextra -O2 -DNDEBUG -s -static -ffunction-sections -fdata-sections -Wl,--gc-sections -o Bit24_c.exe Bit24.c
```

#### C++ 版本

```bash
g++ -std=c++11 -Wall -Wextra -O2 -DNDEBUG -s -static -ffunction-sections -fdata-sections -Wl,--gc-sections -o Bit24_cpp.exe Bit24.cpp
```

产物仅依赖 Windows 系统自带的 `KERNEL32.dll` 与 UCRT（`api-ms-win-crt-*.dll`），体积约 24 KB（C）/ 1.0 MB（C++）。

## 输入格式

- 默认模式：从标准输入读取**一组**四个十进制整数（空格或换行分隔），求解后退出。
  支持任意整数（`int` 范围内），示例：`1 2 3 4`、`10 2 8 5`。
- **`--loop` 模式**：以 `--loop` 为参数启动后，程序会持续读取标准输入，依次处理**所有**组（每组 4 个整数），直到输入结束（EOF）。每行输出一个结果。该模式消除了进程启动开销，主要用于批量求解与性能基准测试（见 [`bench/`](https://github.com/LHM056awa/Bit24/tree/main/bench)）。

## 输出格式

- 有解时：`/24` 后跟表达式（例如 `/24 ((1+2)&3)<<2`）
- 无解时：`No solution`

## 基准测试

项目提供两套 PowerShell 基准脚本，对比 C 与 C++ 实现在不同 `-O` 级别（`-O0`/`-Os`/`-O2`/`-O3`）下的**单次搜索耗时**。

在项目根目录运行（需 PowerShell 5.1+，Windows 10/11 自带）

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File bench\run_bench.ps1            # 串行
powershell -NoProfile -ExecutionPolicy Bypass -File bench\run_bench_parallel.ps1   # 并行
```

### 设计要点

- **隔离单一变量**：只变化 `-O` 级别（两个语言实现统一 `-Wall -Wextra`，不带 `-static`/`-s`/`gc-sections`）。
- **`--loop` 模式摊薄启动开销**：每个可执行文件以 `--loop` 启动一次，进程内连续求解 N 个相同用例（串行默认 1000、并行默认 10000），把进程启动成本摊到可忽略。
- **中位数抗噪**：每组跑 3 轮计时取中位数。
- **三类基准输入**：有解早退出（`1 2 3 4`）、无解含重复数字（`1 1 1 9`）、无解互异负数（`-2 -7 -12 -19`，位运算被拦截、逼近完整搜索空间）。

### 两个脚本的分工

| 脚本 | 模式 | 适用场景 |
| --- | --- | --- |
| `run_bench.ps1` | 串行（单搜索进程独占 CPU） | 测**绝对耗时** |
| `run_bench_parallel.ps1` | 8 线程（C/C++ × 4 个 `-O` 级并发） | 测**横向对比**（C vs C++、`-O` 之间） |

### 已知结论（MinGW GCC 16）

- `-O0`/`-Os` 下 C 与 C++ 无实质差别（比值 ≈ 1.0）；
- `-O2`/`-O3` 下 C++ 稳定快约 15~25%，属**编译器优化管道的工具链级差异**；
- 绝对量级：单次无解搜索约 0.3~1.2 ms，有解时 <0.02 ms，日常使用两者均为「瞬间出结果」。

结果输出：`bench/results.csv`（串行）、`bench/results_parallel.csv`（并行）；各 `-O` 级别的可执行文件按需生成。

## 注意事项

- 除法使用浮点数判断，允许中间步骤产生非整数结果。
- 位运算要求两个操作数都是非负整数（无小数部分），否则该组合被跳过。
- 移位运算的右操作数必须在 0~63 之间，且结果保持在 64 位无符号整数范围内。
