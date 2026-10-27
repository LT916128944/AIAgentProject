# Codex 技能发现

Codex 只自动发现 `.agents/skills/`，不读 `.codex/skills/`。

正文在 `.codex/skills`。本机执行：

```powershell
powershell -ExecutionPolicy Bypass -File .codex\link_agents_skills.ps1
```

这会把 `.agents/skills` 联到 `.codex/skills`。联接目录不提交，避免技能再进一次 git。
