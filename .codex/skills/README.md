# UE5.8 Low Token Skills

给 Codex 的技能正文。规则权威：项目根 `AGENTS.md`。Codex 自动发现的是 `.agents/skills`（联接到这里），不是本目录本身。

改完同步到 `.cursor/skills` 同名文件。`ue-cpp-plan-skill`、`ue-rag-route-skill`、`ue-manual-blueprint-skill`、`ue-history-skill` 也在这里，不挂 MCP 索引块。不要复制本 README 进会话。

原则：Skill 短；已知工具直接调；未知才最小发现；≥2 步走 programmatic；改后只验一次。域专属长流程放到同目录旁路文件（如 `ue-actor-skill/attach.md`），主 Skill 不加载。
