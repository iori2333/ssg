# pack_tool — GIAN07 ENEMY.DAT 修改工具

对 `bin/ENEMY.DAT` 压缩包文件进行解包、分析和精确修改。

## 构建

```bat
# Windows（从 x64_x86 Cross Tools Command Prompt）
build_windows.bat

# Linux
./build_linux.sh
```

输出：`build/bin/pack_tool.exe`（Windows）或 `build/bin/pack_tool`（Linux）。

---

## 基本操作

### 解包

```sh
pack_tool extract bin/ENEMY.DAT work/
# 生成 work/000.bin ~ work/047.bin
```

### 打包

```sh
pack_tool pack work/ ENEMY_MODIFIED.DAT
```

### 备份

修改前务必备份原始文件：

```sh
cp bin/ENEMY.DAT bin/ENEMY_ORIG.DAT
```

---

## 数据文件结构

ENEMY.DAT 包含 48 个条目，每个条目经 LZSS 压缩：

| 条目 | 类型 | 用途 | 加载位置 (LOADER.cpp) |
|------|------|------|----------------------|
| 000–005 | ECL | Stage 1–6 敌方脚本 | `stage + 0 - 1` (行 684) |
| 006–011 | SCL | Stage 1–6 关卡脚本 | `stage + 6 - 1` (行 689) |
| 012–017 | Map | Stage 1–6 地图数据 | `stage + 12 - 1` (行 694) |
| 018–023 | Demo | Stage 1–6 Demo 回放（录制的玩家输入帧） | `stage - 1 + 18` (行 1286) |
| 024–026 | ECL/SCL/Map | Extra Stage | 固定 24/25/26 (行 656–666) |
| 027–046 | Text | 音乐室评论（20 首曲目，Shift-JIS） | `27 + no` (行 1282) |
| 047 | SCL | Ending 脚本 | 固定 47 (行 671) |

---

## 查看数据

### SCL 反汇编

```sh
pack_tool dump-scl work/006.bin    # Stage 1 关卡脚本
```

输出示例：
```
   258  +0x063A  TIME  3930
   259  +0x063F  BOSS  x=319 y=-60 id=0       ← 道中 BOSS
   282  +0x06BB  BOSSDEAD                       ← 道中超时
   496  +0x0C3C  BOSS  x=319 y=-60 id=7       ← 关底 BOSS
   497  +0x0C42  WAITEX  cond=0 opt=0           ← 等待击败
   593  +0x0F6D  STAGECLEAR                     ← 关卡结束
```

关底 BOSS 的 ECL 脚本 ID 见 SCL 的 `BOSS id=N`。

### ECL 概览

```sh
pack_tool dump-ecl work/000.bin     # 所有脚本概览
```

输出示例：
```
ECL header: 10 scripts
  Script 0 @+0x002C: HP=1000 Score=222220
  ...
  Script 7 @+0x0277: HP=6100 Score=1000000
```

### ECL 完整反汇编

```sh
pack_tool dump-ecl work/000.bin 7   # Script 7 的完整字节码
```

输出示例：
```
Script 7 @+0x0277: HP=6100 Score=1000000
  +0x0277: 00 D4 17 00 00 40 42 0F 00  SETUP  HP=6100 Score=1000000
  +0x0280: 0C 23 03 00 00 01 24 13...  STI  STI HP val=4900    ← HP<4900 切阶段
  +0x031A: 00 24 13 00 00 40 42 0F 00  SETUP  HP=4900 Score=1000000
  +0x0323: 0C 29 04 00 00 01 52 03...  STI  STI HP val=850     ← HP<850 切阶段
  +0x0420: 00 52 03 00 00 40 42 0F 00  SETUP  HP=850 Score=1000000
  +0x0429: 0D 01                        CLI                         ← 无更多阶段
  +0x0488: 00 00 00 00 00 00 00 00 00  SETUP  HP=0 Score=0       ← 死亡
```

---

## 修改数据

### 全脚本 SETUP HP 缩放

```sh
pack_tool ecl work/000.bin 1.5 work/000.bin
```

修改该 ECL 文件内**所有**脚本的起始 HP（SETUP 命令的第一个字段）。
去重共享偏移的脚本（难度变体）。

### 关底 BOSS 完整修改

```sh
pack_tool ecl-boss work/000.bin 7 1.5 2.0 work/000.bin
#                              ^      ^   ^
#                          ECL文件 脚本ID HP倍率 时限倍率
```

修改指定脚本的：
- **后续阶段 SETUP HP**（第一个阶段 HP 由 `ecl` 命令处理）
- **STI HP 阈值**（阶段切换血量点）
- **STI TIMER**（阶段时限）

解析器在遇到 death SETUP（HP=0）时自动停止，不会污染共享子程序。

### STI TIMER 单独修改

```sh
pack_tool ecl-time work/002.bin 12 2.0 work/002.bin
```

### 精确字节修补

```sh
pack_tool patch4 work/000.bin 0x278 9150 work/000.bin
#                             ↑       ↑
#                         偏移(hex) 新值(decimal)
```

在指定文件偏移处写入 4 字节 LE uint32。

---

## 关底 BOSS 脚本 ID 速查

以下为各关关底 BOSS 对应的 ECL 脚本 ID，可通过 `dump-scl` 在 SCL 数据中确认：

| 关卡 | ECL 文件 | BOSS 脚本 ID |
|------|---------|-------------|
| Stage 1 | 000.bin | 7 |
| Stage 2 | 001.bin | 10, 12 |
| Stage 3 | 002.bin | 12 |
| Stage 4 | 003.bin | 9 |
| Stage 5 | 004.bin | 13 |
| Stage 6 | 005.bin | 0, 7, 16 |

---

## 完整修改流程示例

目标：所有敌人 HP ×1.5，关底 BOSS 阶段切换血量 ×1.5，阶段时限 ×2.0。

```sh
# 1. 备份
cp bin/ENEMY.DAT bin/ENEMY_ORIG.DAT

# 2. 解包
pack_tool extract bin/ENEMY.DAT work/

# 3. 全脚本 SETUP HP ×1.5
pack_tool ecl work/000.bin 1.5 work/000.bin  # Stage 1
pack_tool ecl work/001.bin 1.5 work/001.bin  # Stage 2
pack_tool ecl work/002.bin 1.5 work/002.bin  # Stage 3
pack_tool ecl work/003.bin 1.5 work/003.bin  # Stage 4
pack_tool ecl work/004.bin 1.5 work/004.bin  # Stage 5
pack_tool ecl work/005.bin 1.5 work/005.bin  # Stage 6

# 4. 关底 BOSS 后续阶段 HP + STI 阈值 + 时限
pack_tool ecl-boss work/000.bin 7  1.5 2.0 work/000.bin  # Stage 1
pack_tool ecl-boss work/001.bin 10 1.5 2.0 work/001.bin  # Stage 2 #1
pack_tool ecl-boss work/001.bin 12 1.5 2.0 work/001.bin  # Stage 2 #2
pack_tool ecl-boss work/002.bin 12 1.5 2.0 work/002.bin  # Stage 3
pack_tool ecl-boss work/003.bin 9  1.5 2.0 work/003.bin  # Stage 4
pack_tool ecl-boss work/004.bin 13 1.5 2.0 work/004.bin  # Stage 5
pack_tool ecl-boss work/005.bin 0  1.5 2.0 work/005.bin  # Stage 6 #1
pack_tool ecl-boss work/005.bin 7  1.5 2.0 work/005.bin  # Stage 6 #2
pack_tool ecl-boss work/005.bin 16 1.5 2.0 work/005.bin  # Stage 6 #3

# 5. 打包并替换
pack_tool pack work/ bin/ENEMY.DAT

# 6. 清理
rm -rf work/
```

恢复原始数据：
```sh
cp bin/ENEMY_ORIG.DAT bin/ENEMY.DAT
```

---

## script_tool — ECL/SCL 反编译 / 编译器

将二进制 ECL/SCL 脚本转换为可读的类汇编文本，并可重新编译回二进制。完整往返——反编译后重新编译的二进制与原文件逐字节一致。

### 构建

包含在标准 CMake 构建中。输出 `build/bin/script_tool(.exe)`。

### 用法

```sh
script_tool disasm-scl <in_binary> <out_text>
script_tool asm-scl   <in_text> <out_binary>
script_tool disasm-ecl <in_binary> <out_text>
script_tool asm-ecl   <in_text> <out_binary>
```

### SCL 文本格式

命名操作数，每个指令一行：

```
TIME frame=1800
ENEMY x=100 y=200 id=5
EFC type=WARN
MSG "Hello World"
MWOPEN
KEY
BOSSDEAD
STAGECLEAR
END
```

非 ASCII 字节使用 `\xNN` 转义。EFC 类型使用符号名（WARN、STG2BOSS 等）。

### ECL 文本格式

使用 `.header`、`.offset`、`.org` 指令保留精确的二进制布局。`@label_XXXX` 引用跳转目标：

```
.header 10
.offset 0 0x002C
.offset 1 0x00FC

.org 0x002C
@script_0:
    SETUP hp=8000 score=5000
    STI jmp=@label_00DC vector=HP val=150
    ANM pattern=0 speed=16
    JMP jmp=@script_1
```

STI vector 类型：`BOSSLEFT`（剩余 Boss 数量）、`HP`（血量阈值）、`TIMER`（帧数）、`BITLEFT`。

### 完整工作流示例

```sh
# 1. 解包 ENEMY.DAT
pack_tool extract bin/ENEMY.DAT work/

# 2. 反编译所有脚本
script_tool disasm-scl work/006.bin work/006.scl
script_tool disasm-ecl work/000.bin work/000.ecl

# 3. 编辑文本文件...
vim work/006.scl work/000.ecl

# 4. 编译回二进制
script_tool asm-scl work/006.scl work/006.bin
script_tool asm-ecl work/000.ecl work/000.bin

# 5. 打包回 ENEMY.DAT
pack_tool pack work/ bin/ENEMY.DAT
```
