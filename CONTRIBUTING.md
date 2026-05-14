# Contributing Guide (协作指南)

感谢你对本项目的关注！以下是参与协作的基本流程。

## 分支策略

- `main` — 稳定分支，始终保持可编译可运行
- `开发分支` — 每个功能或修复在独立分支上开发，完成后合并到 main

### 开发流程

```bash
# 1. Fork 本仓库 (如果是外部贡献者)

# 2. 克隆到本地
git clone https://github.com/<your-username>/Learning-Application-Contest.git

# 3. 创建功能分支
git checkout -b feature/your-feature-name

# 4. 开发完成后提交
git add <files>
git commit -m "feat: 简要描述你的修改"

# 5. 推送到远程
git push origin feature/your-feature-name

# 6. 在 GitHub 上创建 Pull Request
```

## 提交信息规范

请使用语义化的提交信息格式：

| 前缀 | 说明 |
|------|------|
| `feat:` | 新功能 |
| `fix:` | 修复 Bug |
| `refactor:` | 重构 (不改变功能) |
| `perf:` | 性能优化 |
| `docs:` | 文档更新 |
| `style:` | 代码格式调整 (不影响逻辑) |
| `test:` | 添加或修改测试 |
| `chore:` | 构建/工具链变更 |

示例：`feat: 添加按体积排序的放置策略`

## 代码规范

- **C++ 标准**: C++17
- **编码**: UTF-8 (源文件)、GBK (控制台输出)
- **命名风格**:
  - 类名: `PascalCase` (如 `PalletAlgorithm`)
  - 成员变量: `snake_case_` (带下划线后缀，如 `totalGrabs_`)
  - 函数名: `PascalCase` (如 `ProcessQueue`)
  - 常量: `UPPER_SNAKE_CASE` (如 `MAX_WIDTH`)
  - 局部变量: `snake_case` (如 `best_x`)
- **头文件保护**: 使用 `#pragma once` + 传统 `#ifndef` 双重保护
- **注释语言**: 中文或英文均可，但同一文件内保持一致

## 项目结构规范

- 源代码放在 `code/` 目录下
- 数据文件 (`dims.csv`, `orders.tsv`) 放在 `code/` 目录下
- 不要提交编译产物 (`.obj`, `.exe`, `.lib` 等)
- 不要提交个人临时文件

## 报告问题

如果发现 Bug 或有功能建议，请通过 [Issues](../../issues) 反馈，并尽量包含：

1. 问题的描述
2. 复现步骤
3. 期望行为 vs 实际行为
4. 你的编译环境 (VS 版本、EasyX 版本等)
