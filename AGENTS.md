# AGENTS.md - UE5.8 MCP Token 节流

> 唯一权威。`.codex/AGENTS.md` 只是指针。
> 成本：**返回值比轮数更贵**（永久留在上下文）。少往返 + 少返回。
> 事实库 `.codex/skills/ue-low-token-skill/references/ue_mcp_verified.json`（只 `rg` 窄查，禁整文件）。索引 `ue_mcp_index.json`（1.4MB，禁注入）。新事实必须回写事实库。
> 域 Skill 只写：工具路径、必填参数、本域已验证坑。铁律不复述。

## 铁律（九条，语义不可删）

1. **禁探索**：禁 `list_toolsets`；禁 `tool_search`；禁 `GetDynamicTools` 扫 unreal 命名空间；禁简称试探；禁拉整份 Schema。编辑器工具全名/必填先跑 `scan_mcp_tools.py`，没有走「求助」。
2. **禁全量**：编辑器工具参数只 `--schema` 一个 `full_name`。索引/事实库只窄切片，禁整文件进上下文。禁读 `.uasset` / `.umap` / `Binaries` / `Intermediate` / `Saved` / `DerivedDataCache` / `.vs`。
3. **强制批处理**：同一编辑器任务 ≤8 次 `call_mcp_tool.py`。不为批处理去发现 `execute_tool_script` 或整份 Toolset。
4. **遇错即停**：最多重试一次，失败原文交用户。`Toolset not found` → `regenerate_index.ps1`。
5. **只验证一次**：只验用户要的那项，成功即停。编辑器能验就不 PIE。用户说不用运行/不用保存 → 禁 StartPIE、禁保存。
6. **默认保存**：写场景后保存当前关卡；任务显式 `save: false` 才只改内存。缺省由 `UE_DEFAULT_SAVE` 控制（缺省 true）。
7. **输出最小化**：工具/脚本返回只留决策字段（约 200 字），长串截断。
8. **资产 RAG 只读**：`search_ue_project` / collection `ue_assets` 只检索，不写项目、不改关卡。编辑器读写走扫描脚本，不走全量 MCP 发现。任务显式 `save: false` / `pie: false` 时才禁保存、禁 PIE。
9. **语义查询必须 RAG 先行**：无确切资产路径/类名时，第一动作是 `search_ue_project`，调用前禁止列目录、读文件或调其他工具。返回可用就据此回答；为空或不相关才回退文件扫描，且不为此拉 unreal 全量工具。

## 路由表

| 你要做的事 | 打开 |
| --- | --- |
| 动编辑器 | `.env` 里 `UE_EDITOR_BACKEND=monolith` 时打开 `ue-monolith-backend-skill`；未写或 `official` 时打开 `.codex/skills/ue-rag-route-skill/SKILL.md`，第一动作 `scan_mcp_tools.py` |
| 改 C++ 或会落到 UE 工程 | `.codex/skills/ue-cpp-plan-skill/SKILL.md` |
| 蓝图 / 材质 / UMG / 动画 / Niagara 连线 | 先看对应 `UE_ALLOW_*`；false 则 `ue-manual-blueprint-skill` 写 `Plan/` |
| 报错、不生效、查不到 | `search_ue_project`，`doc_kind=history`，`top_k=1` |
| 架构边界和风险分级 | `documents/harness/boundaries.md` |
| 当前做到哪 | `RAG/ue-rag/data/harness/progress.json` |
| 功能是否真完成 | `RAG/ue-rag/data/harness/feature_list.json` |
| 某条操作的细节 | `WORKFLOW.md` 里对应标题，不要全文 |

技能正文只在 `.codex/skills`。Cursor 用 `.cursor/skills` 镜像，Codex 用 `.agents/skills` 联接。域列表：actor / animation / asset / blueprint / debug / material / niagara / object / pcg / sequencer / static-mesh / umg / verify / programmatic / cpp-plan / low-token。跨域只取真正用到的那几节。引擎知识：`rg -n -A 20 '^## <标题>' .codex/skills/ue-low-token-skill/references/ext_ue5_knowledge.md`。

## MCP 调用

工作目录 `E:\UEProject\AIWork\RAG\ue-rag`，Python 用该目录下 `.venv\Scripts\python.exe`。

- 扫描（卡片，无 `input_schema`）：`python scripts/scan_mcp_tools.py --query "<自然语言>" --top-k 8`
- 单个 schema：`python scripts/scan_mcp_tools.py --schema "<full_name>"`
- 调用：`python scripts/call_mcp_tool.py --toolset "<toolset>" --tool "<裸名>" --args "<json>"`
- 直连 `http://127.0.0.1:8000/mcp`。检索不相关就换一种说法再扫一次；仍没有则停，不回退全量 schema。

`search_ue_project(query, top_k=5)`（命名空间 `mcp_ue_rag`，独立 server）：`metadata` 含 `source_type / asset_path / class_name / display_name`，一次调用即得「有什么资产 + 在哪 + 是什么类」。模糊查询用，不传确切路径和类全名。概览 `get_project_summary()` 无参。

## 求助

工具扫描换一个说法再扫一次仍不相关：**停**，只说——"当前任务需要 [动作]，未知 MCP 工具全路径和 JSON 参数。"不回退全量 schema，不 `tool_search`。调用成功后的新坑回写 `ue_mcp_verified.json`（只 `rg` 窄查）与对应 Skill。

资产检索为空或不相关不算求助，可回退文件扫描，不为此拉 unreal 全量工具。

## Codex

Codex 读本文件，不读 `.cursor/rules/*.mdc`。同等短文在 `.codex/instructions/`，由 `.codex/hooks.json` 的 `SessionStart` 注入；钩子没注入时，开工先读这四份短文。技能正文在 `.codex/skills`；自动发现要 `.agents/skills`（跑 `.codex/link_agents_skills.ps1`）。计划、约定、给人看的任务记录两边共用 `.cursor/memory`。写图拦截：Cursor 用 `.cursor/hooks.json`，Codex 用 `PreToolUse`。钩子要在 Codex 里 `/hooks` 信任后才跑。

## 维护

改 Skill / `AGENTS.md` 直接写文件。YAML 从文件首字节 `---` 开始、无 BOM。索引块由 `apply_index_block.ps1` 生成，勿手改块内。改完跑 `check_skills.ps1`。

域 Skill 正文以 `.codex/skills` 为源；改完必须同步到 `.cursor/skills` 同名文件。`ue-cpp-plan-skill`、`ue-rag-route-skill`、`ue-manual-blueprint-skill`、`ue-history-skill` 同样两边各一份。`.cursor/rules` 与 `.codex/instructions` 成对，改短文时两处一起改。`.cursor/memory` 仍是唯一的人读记忆。

`Plan/终极蒸馏方案/升级执行/` 里 `pre_authorized: true` 的文件，覆盖「先问再改」，范围只等于该文件的允许写入列表。

## DeepSeek Harness（2026-10-23 起，第二入口）

- 入口：DSH 打开工作区 `E:\UEProject\AIWork`；配置模板 `RAG/ue-rag/dsh/cordis.patch.yml`，说明 `RAG/ue-rag/dsh/README.md`。
- 工具：只经 `mcp__uerag__*` 调检索/执行/裁判；禁直连官方 830 工具，禁在 DSH 里拉全量 schema。
- 写策略：`allow_writes` / `save` / `pie` 只从 `RAG/ue-rag/data/dsh/session_task.json` 读，该文件只由人写；模型参数里没有这些字段。
- 证据：`read_contract` + `verify` 是唯一可引用的完成证据；`passed` 仍只由裁判改写，`manual` 永不由模型改。
- DSH 会话开工按 `ue-dsh-session-skill`；本文件和九条铁律在 DSH 会话里同样有效。移植策略见 `Plan/20261023-1520-DeepSeekHarness深度绑定.md`。
- 说完成之前先 `verify`；没有 `{ok:true, kind}` 的条款就是没完成。
- `manual` 条款永远不由模型改；由人在 `Plan/` 里绑。
- DSH 会话不写 `allow_writes`、不写 `save/pie`、不写 `.env`。