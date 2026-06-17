# Stage 5 — 静态分析配置（1 个 PR）

## 目标
- 添加 `.clang-format` 配置文件。
- 添加 `.clang-tidy` 配置文件。
- 添加 GitHub Actions / pre-commit hook 强制运行。

**风险等级**：低（仅配置文件，不改代码）。

---

## PR 5.1：添加静态分析配置

### 任务清单

#### 1. 创建 `.clang-format`

**位置**：项目根目录 `C:\Users\Iori\Workspace\ssg\.clang-format`。

**内容**（基于 LLVM 风格 + 现有 2 空格缩进）：

```yaml
---
BasedOnStyle: LLVM
IndentWidth: 2
ColumnLimit: 100
UseTab: Never
TabWidth: 2
NamespaceIndentation: None
AccessModifierOffset: -2
ContinuationIndentWidth: 4

Language: Cpp
Standard: c++20

AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: Never
AllowShortLambdasOnASingleLine: Inline
AllowShortLoopsOnASingleLine: false

BraceWrapping:
  AfterCaseLabel: false
  AfterClass: true
  AfterControlStatement: Always
  AfterEnum: true
  AfterFunction: true
  AfterNamespace: false
  AfterObjCDeclaration: false
  AfterStruct: true
  AfterUnion: true
  AfterExternBlock: false
  BeforeCatch: true
  BeforeElse: true
  BeforeLambdaBody: false
  BeforeWhile: false
  IndentBraces: false
  SplitEmptyFunction: true
  SplitEmptyRecord: true
  SplitEmptyNamespace: true

AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignEscapedNewlines: Right
AlignOperands: true
AlignTrailingComments: true

KeepEmptyLinesAtTheStartOfBlocks: false
MaxEmptyLinesToKeep: 1
ReflowComments: true

IncludeBlocks: Preserve
IncludeCategories:
  - Regex: '^<.*\.h>'
    Priority: 1
  - Regex: '^<.*'
    Priority: 2
  - Regex: '.*'
    Priority: 3
SortIncludes: false

PointerAlignment: Left
```

**说明**：
- `IndentWidth: 2` — 匹配现有代码（2 空格缩进）。
- `ColumnLimit: 100` — 现代 C++ 习惯（100 列）。
- `BraceWrapping.AfterFunction: true` — 函数左大括号独占一行（匹配现有风格）。
- `BraceWrapping.AfterStruct: true` — 结构体左大括号独占一行（匹配现有风格）。
- `IncludeBlocks: Preserve` — 不强制 include 顺序。
- `SortIncludes: false` — 不强制 include 排序（避免大量改动）。

#### 2. 创建 `.clang-tidy`

**位置**：项目根目录 `C:\Users\Iori\Workspace\ssg\.clang-tidy`。

**内容**：

```yaml
---
Checks: >
  -*,
  bugprone-*,
  -bugprone-easily-swappable-parameters,
  -bugprone-narrowing-conversions,
  -bugprone-reserved-identifier,
  cert-*,
  -cert-dcl37-c,
  -cert-dcl51-cpp,
  -cert-err33-c,
  cppcoreguidelines-*,
  -cppcoreguidelines-avoid-magic-numbers,
  -cppcoreguidelines-avoid-non-const-global-variables,
  -cppcoreguidelines-pro-bounds-array-to-pointer-decay,
  -cppcoreguidelines-pro-bounds-constant-array-index,
  -cppcoreguidelines-pro-bounds-pointer-arithmetic,
  -cppcoreguidelines-pro-type-cstyle-cast,
  -cppcoreguidelines-pro-type-reinterpret-cast,
  -cppcoreguidelines-pro-type-union-access,
  -cppcoreguidelines-pro-type-vararg,
  -cppcoreguidelines-special-member-functions,
  modernize-*,
  -modernize-avoid-c-arrays,
  -modernize-deprecated-headers,
  -modernize-make-shared,
  -modernize-make-unique,
  -modernize-pass-by-value,
  -modernize-raw-string-literal,
  -modernize-return-braced-init-list,
  -modernize-use-auto,
  -modernize-use-bool-literals,
  -modernize-use-default-member-init,
  -modernize-use-equals-default,
  -modernize-use-equals-delete,
  -modernize-use-nodiscard,
  -modernize-use-trailing-return-type,
  performance-*,
  readability-*,
  -readability-braces-around-statements,
  -readability-isolate-declaration,
  -readability-magic-numbers,
  -readability-named-parameter,
  -readability-redundant-declaration,
  -readability-redundant-string-init,
  -readability-static-accessed-through-instance,
  -readability-else-after-return,
  -readability-implicit-bool-conversion,
  -readability-identifier-length,

HeaderFilterRegex: '.*\.(h|hpp)$'
ImplementationFileExtensions:
  - cpp
  - cxx
  - cc

WarningsAsErrors: ''

ExtraArgs:
  - -std=c++20
  - -I.
```

**说明**：
- 启用了 `bugprone-*`、`modernize-*`、`performance-*`、`readability-*`、`cppcoreguidelines-*`（部分）。
- 禁用了部分过度严格的检查（如 `modernize-use-trailing-return-type` 强制 `auto f() -> int` 风格）。
- `HeaderFilterRegex` — 仅 lint 项目头文件，跳过第三方。

#### 3. 创建 `pre-commit` 配置（可选）

**位置**：`.pre-commit-config.yaml`（如果项目使用 pre-commit）。

**内容**：

```yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.4.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-added-large-files

  - repo: https://github.com/pocc/pre-commit-hooks
    rev: v1.3.5
    hooks:
      - id: clang-format
        files: '\.(cpp|h|hpp|c|cc)$'
```

**注意**：项目当前未使用 pre-commit（根据 `AGENTS.md` 无相关描述）。本 PR 仅添加配置文件，不强制 pre-commit 安装。

#### 4. 创建 GitHub Actions 工作流

**位置**：`.github/workflows/static-analysis.yml`。

**内容**：

```yaml
name: Static Analysis

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main, develop]

jobs:
  clang-format:
    name: clang-format
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install clang-format
        run: sudo apt-get install -y clang-format

      - name: Check formatting
        run: |
          find GIAN07 game platform -name "*.cpp" -o -name "*.h" | \
            xargs clang-format --dry-run --Werror

  clang-tidy:
    name: clang-tidy
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install clang-tidy
        run: sudo apt-get install -y clang-tidy

      - name: Run clang-tidy
        run: |
          # 仅在 PR 上跑（避免 main 上无意义的失败）
          if [ "${{ github.event_name }}" = "pull_request" ]; then
            find GIAN07 -name "*.cpp" | head -20 | \
              xargs -I {} clang-tidy {} -p build --warnings-as-errors=*
          fi
```

**说明**：
- `clang-format` 检查：失败时阻止 PR 合并。
- `clang-tidy` 仅在 PR 上跑（前 20 个文件，避免 CI 慢）。
- Windows 构建本地跑。

#### 5. 更新 `AGENTS.md` / `README.md`

在 `AGENTS.md` 添加：

```markdown
## 静态分析

- `.clang-format` 强制代码风格（100 列、2 空格缩进、大括号独占行）
- `.clang-tidy` 启用 bugprone / modernize / performance / readability 子集
- 运行：`clang-format -i GIAN07/<file>.cpp` 自动格式化
- 运行：`clang-tidy GIAN07/<file>.cpp -- -std=c++20` 检查
```

---

## 验证

### 本地验证

```bash
# 1. clang-format 检查
clang-format --dry-run --Werror GIAN07/bullet/TAMA.cpp

# 2. clang-tidy 检查
clang-tidy GIAN07/bullet/TAMA.cpp -- -std=c++20 -I.

# 3. 自动修复
clang-format -i GIAN07/bullet/TAMA.cpp
clang-tidy --fix GIAN07/bullet/TAMA.cpp -- -std=c++20
```

### CI 验证（GitHub Actions）

提交到分支后，GitHub Actions 应自动运行。

---

## 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 配置文件与现有风格不一致 | 大量改动提示 | 阶段 1-4 完成后启用，存量代码用 `clang-format` 一次性格式化 |
| `clang-tidy` 警告数过多 | CI 失败 | 阶段 5 初期不启用 `-warnings-as-errors`，先观察 |
| 配置文件影响 IDE（CLion/VS） | 开发者体验变化 | 文档化使用方法 |
| `.clang-tidy` 启用 `cppcoreguidelines-pro-type-cstyle-cast` | 大量警告 | 已禁用 |

---

## 提交策略

1 个 PR，包含：
- `.clang-format`
- `.clang-tidy`
- `.pre-commit-config.yaml`（可选）
- `.github/workflows/static-analysis.yml`（可选）
- `AGENTS.md` 更新

**预期 commit 数**：1-2（配置文件 + 文档）。

---

## 与阶段 1-4 的协调

**建议时机**：
- **选项 A**：阶段 5 在阶段 4 之后（即文件名稳定后）。
- **选项 B**：阶段 5 在阶段 1 之后（即风格打磨后，但文件名仍是大写）。
- **选项 C**：阶段 5 在阶段 1 之后立即执行 `clang-format` 一次性格式化所有当前文件。

**推荐**：选项 C — 阶段 1 完成后立即格式化。这样后续 PR 都基于格式化后的代码。

但这会**一次性大规模改动**（所有 100+ 个文件）。如果团队希望保留 blame 历史的可读性，可以分批：
- 先加配置文件（不强制）
- 后续 PR 中增量应用

---

## 后续阶段衔接

完成阶段 5 后：
- 阶段 6（可选优化）开始
- 所有后续 PR 自动应用 `.clang-format`
- CI 自动检查代码质量
