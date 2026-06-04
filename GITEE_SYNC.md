# GitHub/Gitee 同步说明

本仓库的主维护仓库建议放在 GitHub，Gitee 可以作为国内访问镜像。当前公开说明不写死 Gitee 地址，避免读者误以为某个镜像一定存在或一定同步。

## 推荐维护方式

| 平台 | 定位 |
| --- | --- |
| GitHub | 主仓库，负责 issue、release、CI、主要提交记录 |
| Gitee | 镜像仓库，方便国内访问和备份 |

如果后续创建 Gitee 镜像，建议在 `README.md` 中补充明确链接，并说明以 GitHub 为准。

## 手动同步流程

假设已经创建 Gitee 空仓库，可以按下面方式添加远程：

```bash
git remote add gitee https://gitee.com/<your-name>/Embedded-Career-Roadmap.git
```

同步主分支：

```bash
git push gitee main
```

同步标签：

```bash
git push gitee --tags
```

查看远程：

```bash
git remote -v
```

## 日常维护建议

1. 先在 GitHub 主仓库完成提交、CI 检查和 release。
2. 确认 GitHub `main` 分支状态正常后，再同步到 Gitee。
3. Gitee 仓库说明中写清楚“镜像仓库，更新可能滞后”。
4. 如果 Gitee 不跑 GitHub Actions，同步说明中不要声称 Gitee 有同等 CI 验证。
5. 如果两个平台都有 issue，建议明确只在一个平台集中处理，避免问题分散。

## README 中可使用的说明

```text
GitHub 为主仓库，Gitee 作为国内访问镜像。若两个平台内容存在差异，以 GitHub 最新提交和 Release 为准。
```

## 边界说明

- 不要在 Gitee 镜像中加入 GitHub 没有的真实公司项目资料。
- 不要在镜像仓库中改写硬件接线、端子定义或设备寄存器说明。
- 如果镜像仓库由他人 fork 或转载，仍应保留原许可证和来源说明。
