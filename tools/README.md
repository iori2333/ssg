# pack_tool — GIAN07 pack file 修改工具

对 PBG 压缩包文件（`*.DAT`, `*.PAK`）进行解包、分析和修改。

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

### MAP.PAK（地图/回放包）

MAP.PAK 包含 13 个条目（编号 000–012），由旧 48 条目 ENEMY.DAT 精简而来。
脚本条目已在编译期嵌入二进制，音乐室评论已迁移至 MUSIC.PAK。

| 条目 | 类型 | 用途 | 加载位置 (LOADER.cpp) |
|------|------|------|----------------------|
| 000–005 | Map | Stage 1–6 地图数据 | `stage - 1` |
| 006–011 | Demo | Stage 1–6 Demo 回放 | `stage - 1 + 6` |
| 012 | Map | Extra Stage 地图数据 | 固定 12 |

### 旧 ENEMY.DAT → MAP.PAK 转换对照

| 旧条目 | → 新条目 | 内容 |
|--------|---------|------|
| 012–017 | 000–005 | Stage 1–6 地图 |
| 018–023 | 006–011 | Stage 1–6 Demo |
| 026 | 012 | Extra Stage 地图 |

### IMAGES.PAK（合并图像包）

由 `GRAPH.DAT` 和 `GRAPH2.DAT` 合并而成，共 39 个条目（编号 000–038）。

| 条目 | 来源 | 用途 |
|------|------|------|
| 000–031 | GRAPH.DAT（原索引不变） | 系统界面、敌人精灵、地图 tiles、脸部、UI |
| 032 | GRAPH2.DAT 条目 0 | Ending staff roll 背景 |
| 033–038 | GRAPH2.DAT 条目 1–6 | Ending CG 图 ×6 |

### MUSIC.PAK（统一音乐包）

每个条目载荷 = `[title_len:u32LE][title:UTF-8][comment_len:u32LE][comment:UTF-8\n分隔][midi_data:原始SMF]`

由 `pack_tool extract-music` / `pack_tool pack-music` 工具链生成。

---

## 音乐数据迁移（GBK → UTF-8）

将原始 GBK 编码的曲名和评论转换为统一 UTF-8 格式的 `MUSIC.PAK`：

```sh
# 1. 从 MUSIC.DAT + 旧 ENEMY.DAT 提取并转换
pack_tool extract-music bin/ music_work/

# 输出 music_work/track_00/ ~ track_19/，每目录包含：
#   midi.mid    原始 SMF MIDI 文件
#   title.txt   曲名（UTF-8）
#   comment.txt 评论（UTF-8，多行 \n 分隔）

# 2. 打包为统一格式
pack_tool pack-music music_work/ bin/MUSIC.PAK

# 3. 旧 ENEMY.DAT → MAP.PAK（提取 013.bin→000.bin 等，重打包）
pack_tool extract bin/ENEMY.DAT /tmp/mapwork/
cp /tmp/mapwork/012.bin /tmp/mapwork/000.bin  # ... 自动或手动映射
pack_tool pack /tmp/mapwork/ bin/MAP.PAK
```

### 一键迁移流程

```sh
# 音乐
pack_tool extract-music bin/ music_work/
pack_tool pack-music music_work/ bin/MUSIC.PAK

# 地图/回放（旧 ENEMY.DAT → MAP.PAK）
pack_tool extract bin/ENEMY.DAT /tmp/mapwork/
for i in 0 1 2 3 4 5; do cp /tmp/mapwork/$(printf "%03d" $((i+12))).bin /tmp/mapwork/$(printf "%03d" $i).bin; done
for i in 0 1 2 3 4 5; do cp /tmp/mapwork/$(printf "%03d" $((i+18))).bin /tmp/mapwork/$(printf "%03d" $((i+6))).bin; done
cp /tmp/mapwork/026.bin /tmp/mapwork/012.bin
rm /tmp/mapwork/000.bin /tmp/mapwork/001.bin /tmp/mapwork/002.bin /tmp/mapwork/003.bin /tmp/mapwork/004.bin /tmp/mapwork/005.bin
rm /tmp/mapwork/00{6,7,8,9,10,11}.bin
rm /tmp/mapwork/01{3,4,5,6,7}.bin /tmp/mapwork/01{8,9}.bin /tmp/mapwork/02{0,1,2,3}.bin
rm /tmp/mapwork/02{4,5}.bin /tmp/mapwork/027.bin /tmp/mapwork/028.bin
rm /tmp/mapwork/02{9,7,8,9}.bin /tmp/mapwork/03{0,1,2,3,4,5,6,7,8,9,9,9}.bin 2>/dev/null
rm /tmp/mapwork/04{0,1,2,3,4,5,6,7}.bin
pack_tool pack /tmp/mapwork/000.bin /tmp/mapwork/001.bin /tmp/mapwork/002.bin /tmp/mapwork/003.bin /tmp/mapwork/004.bin /tmp/mapwork/005.bin /tmp/mapwork/006.bin /tmp/mapwork/007.bin /tmp/mapwork/008.bin /tmp/mapwork/009.bin /tmp/mapwork/010.bin /tmp/mapwork/011.bin /tmp/mapwork/012.bin
# (上面手动列出所有 13 个文件，pack_tool pack 需要目录或文件列表)
```

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
# 1. 备份（旧 ENEMY.DAT 或已迁移的 MAP.PAK）
cp bin/MAP.PAK bin/MAP_ORIG.PAK

# 2. 解包
pack_tool extract bin/MAP.PAK work/

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
pack_tool pack work/ bin/MAP.PAK

# 6. 清理
rm -rf work/
```

恢复原始数据：
```sh
cp bin/MAP_ORIG.PAK bin/MAP.PAK
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

脚本已从 pack 文件中移出并嵌入二进制（`scripts/` → 编译期 `embedded_scripts[]`）。反编译/编辑的源文件在 `scripts/*.ecl` 和 `scripts/*.scl`，修改后重新构建即可。

```sh
# 1. 反编译嵌在源码中的脚本
script_tool disasm-scl scripts/stage1.scl work/stage1.txt
script_tool disasm-ecl scripts/stage1.ecl work/stage1.txt

# 2. 编辑文本文件...
vim work/stage1.txt

# 3. 编译回二进制
script_tool asm-scl work/stage1.txt scripts/stage1.scl

# 4. 重新构建游戏
./build_windows.bat
```
