# UE5.8 Low Token Skills

给 Codex/Cline 等读取 `.codex/skills` 的客户端。规则权威：项目根 `AGENTS.md`。

把各 Skill 文件夹放到 `.codex/skills/`，保留 `SKILL.md` 文件名。不要复制本 README 进会话。

原则：Skill 短；已知工具直接调；未知才最小发现；≥2 步走 programmatic；改后只验一次。域专属长流程放到同目录旁路文件（如 `ue-actor-skill/attach.md`），主 Skill 不加载。
