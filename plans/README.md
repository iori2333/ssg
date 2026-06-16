# GIAN07 现代 C++ 迁移计划

> 目录索引：本目录包含 7 份计划文件，按阶段顺序排列。

## 文件清单

| 文件 | 阶段 | 描述 | 风险 |
| --- | --- | --- | --- |
| `00-overview.md` | Stage 0 | 盘点与基线（不修改代码） | 0 |
| `01-style-polish.md` | Stage 1 | 风格打磨：去 `(void)` / `extern` / `// was` 注释 / `this->` 冗余（4-7 个 PR） | 低 |
| `02-type-system.md` | Stage 2 | 类型系统修复：`typedef struct` / `bool` / `[[nodiscard]]` / `noexcept`（2 个 PR） | 低-中 |
| `03-api-modernization.md` | Stage 3 | API 现代化：删除 `using` 别名 / `std::function` / 删 `EclInterpreter` / `Indsort` → `ranges`（5 个 PR） | 中 |
| `04-filename-rename.md` | Stage 4 | 8.3 文件名 → snake_case（按子目录分 7 个 PR） | 中-高 |
| `05-static-analysis.md` | Stage 5 | 静态分析配置：`.clang-format` / `.clang-tidy`（1 个 PR） | 低 |
| `06-optimization.md` | Stage 6 | 可选优化：`unique_ptr` / 静态状态 / 单元测试（10+ 个 PR） | 低-中 |

## 总体时间线（建议）

```
Week 1-2:   Stage 0 盘点 → Stage 1 风格打磨（5 个 PR）
Week 3:     Stage 2 类型系统（2 个 PR）
Week 4-5:   Stage 3.1.A using 别名（21 个 commit，1 个 PR）
Week 6-7:   Stage 3.1.B 大影响别名（3 个子 PR）
Week 8:     Stage 3.2-3.4（3 个 PR）
Week 9-11:  Stage 4 子目录重命名（7 个 PR）
Week 12:    Stage 5 静态分析（1 个 PR）
Week 13+:   Stage 6 可选优化（10+ 个 PR）
```

**总计 PR 数**：~30 个
**总计 commit 数**：~70-80 个

## 关键决策（已确认）

1. ✅ 8.3 文件名全部重命名（按子目录分批）
2. ✅ 一次性删除 28 个 `using` 别名（连带修改所有调用方）
3. ✅ `GameInit` 函数指针 → `std::function`（保留 `current_state` 字段）
4. ✅ `EffectManager` 保持单 struct（不拆分）
5. ✅ 删除所有 `// was xxx` 迁移注释
6. ✅ `Indsort` → `std::ranges::stable_partition`（保留 `indices[]` 间接访问）
7. ✅ 全局 Manager 单例保留（暂不引入 DI）
8. ✅ 阶段 1.1-1.4 拆分 2-3 个 PR
9. ✅ 阶段 3.1 按"类别/批"提交
10. ✅ 完全删除 `EclInterpreter`
11. ✅ 加 `.clang-tidy` / `.clang-format`

## 通用验证清单（每个 PR 都必须做）

1. **本地构建**：`./build_windows.bat` 0 错误 0 警告。
2. **单元测试**（如果适用）：`ctest --test-dir build` 全通过。
3. **静态分析**：`clang-tidy` 0 新警告。
4. **代码审查**：每个 PR 至少 1 名 reviewer。
5. **游戏冒烟测试**：进入游戏 → 开始关卡 → 触发敌人 → 触发子弹 → 触发 boss → 通关一关。
6. **回归**：与 main branch diff 仅限预期改动。

## 风险登记（汇总）

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 8.3 大小写冲突（Linux CI） | 编译失败 | 阶段 4 每个子目录 PR 后跑 Linux 构建 |
| `using` 别名替换误伤同名字段 | 编译失败 | 全局 grep 二次确认 + 单 PR 单别名小批量 |
| `Indsort` 算法替换改变顺序 | 视觉异常 | 阶段 3.4 加单元测试 + 游戏冒烟 |
| `EclInterpreter` 删除后调用点遗漏 | 链接失败 | 全局 grep 确认 0 引用 + 删除 |
| `std::function` 性能 | 函数调用开销 | `GameInit` 不在热路径，可接受 |
| 重命名后 IDE 跳转失效 | 开发体验 | 让 IDE 重建索引 |
| 静态分析配置过严 | 大量警告 | 初期不强制 -Werror |
| `unique_ptr` 移动语义 | 编译错误 | audit 所有 `BossData` 拷贝 |

## 执行入口

**当前状态**：阶段 0 待执行（盘点）。

**建议下一步**：
1. 创建 `docs/migration/` 目录
2. 跑基线构建
3. 输出 7 份清单文件
4. 开始 Stage 1.1（PR：去除 `(void)` 与 `extern`）

如需调整任何计划，请编辑对应的 `XX-name.md` 文件。

## 状态跟踪

使用 git tag / milestone 跟踪每个 PR：

```bash
# Stage 0 完成
git tag stage-0-complete

# Stage 1 完成（4-7 个 PR 全部合并）
git tag stage-1-complete

# 等等
```

每个 stage 完成后，标记 tag 并更新此 README。
