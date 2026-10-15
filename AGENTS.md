# AGENTS.md - UE5.8 MCP Token 节流

> 唯一权威。`.codex/AGENTS.md` 只是指针。
> 成本：**返回值比轮数更贵**（永久留在上下文）。少往返 + 少返回。
> 事实库 `.codex/skills/ue-low-token-skill/references/ue_mcp_verified.json`（只 `rg` 窄查，禁整文件）。索引 `ue_mcp_index.json`（1.4MB，禁注入）。新事实必须回写事实库。
> 域 Skill 只写：工具路径、必填参数、本域已验证坑。铁律不复述。

## 铁律

1. **禁探索**：禁 `list_toolsets`；禁 `tool_search`；禁 `GetDynamicTools` 扫 unreal 命名空间；禁简称试探 `toolset_name`；禁拉整份 Schema。编辑器工具的全名/必填先跑 `scan_mcp_tools.py`，没有走「求助」。
2. **禁全量**：编辑器工具参数只 `--schema` 一个 `full_name`。索引/事实库只窄切片，禁整文件进上下文。禁读 `.uasset` / `.umap`。禁读 `Binaries` / `Intermediate` / `Saved` / `DerivedDataCache` / `.vs`。
3. **强制批处理**：同一编辑器任务 ≤8 次 `call_mcp_tool.py`。不要为批处理去发现 `execute_tool_script` 或整份 Toolset。
4. **遇错即停**：最多重试一次，失败原文交用户。`Toolset not found` → `regenerate_index.ps1`。
5. **只验证一次**：只验用户要的那项，成功即停。编辑器能验就不 PIE。用户说不用运行/不用保存 → 禁 StartPIE、禁 `save_actor`、禁存关卡。
6. **默认不保存**：未要求则只改内存。
7. **输出最小化**：工具/脚本返回只留决策字段（约 200 字），长串截断。
8. **资产 RAG 只读**：`search_ue_project` / collection `ue_assets` 只检索，不写项目、不改关卡。编辑器读写走 `scan_mcp_tools.py` + `call_mcp_tool.py`，禁止改走会把全量 schema 装进上下文的 MCP 发现。未要求则不保存、不 StartPIE。
9. **语义查询必须 RAG 先行**：无确切资产路径/类名时，第一动作是 `search_ue_project`。调用前禁止列目录、读文件或调其他工具；返回可用就据此回答，为空或不相关才回退文件扫描。不要为此去拉 unreal 全量工具。

## 路由（按 description，禁止全量读 SKILL.md）

**★★★ 最高优先级：要动编辑器**（Actor / 属性 / 验证 / 编译已有蓝图）→ 先读 `ue-rag-route-skill`，第一动作 `scan_mcp_tools.py`。禁止把 MCP schema 装进上下文。
**改蓝图先看开关**：`UE_ALLOW_BLUEPRINT` 缺省 `false`。先读 `ue-manual-blueprint-skill`。`false` 写 `Plan/` 操作步骤，不扫描、不改图。`true` 才回到 `ue-blueprint-skill` 改图，失败到 `max_retries` 停止。材质图、UMG、动画蓝图、Niagara 的连线不论开关，都走人工清单。
**★★★ 资产语义/模糊查询/跨语言 · 无确切路径** → **第一动作直调 `search_ue_project`**（见 MCP 段）。
**在此之后**才考虑以下域：
actor 场景/Transform · animation 骨骼/ControlRig · asset 资产/保存 · blueprint 蓝图/Graph · debug 报错/日志/不生效 · material · niagara · object 属性 · pcg · sequencer · static-mesh · umg · verify 验证 · programmatic 批处理 · cpp-plan 新功能/重构/实施计划 · low-token 跨域/未知工具集。
跨域只读真正需要的那几个。C++ / 蓝图等**将改 UE 项目**的实施计划与完成后落盘 → `ue-cpp-plan-skill`。引擎知识/存档/性能/架构：`rg -n -A 20 '^## <标题>' .codex/skills/ue-low-token-skill/references/ext_ue5_knowledge.md`。

## MCP

编辑器操作不要 `tool_search`，不要把 `call_tool` 的 schema 装进上下文。第一动作是扫描脚本，步骤见 `ue-rag-route-skill`。

工作目录 `E:\UEProject\AIWork\RAG\ue-rag`，Python 用该目录下 `.venv\Scripts\python.exe`。

- 扫描（卡片，无 `input_schema`）：`python scripts/scan_mcp_tools.py --query "<自然语言>" --top-k 8`
- 单个 schema：`python scripts/scan_mcp_tools.py --schema "<full_name>"`
- 调用：`python scripts/call_mcp_tool.py --toolset "<toolset>" --tool "<裸名>" --args "<json>"`

直连 `http://127.0.0.1:8000/mcp`。返回只留决策字段（约 200 字）。检索不相关就换一种说法再扫一次；仍没有则停，不回退全量 schema。

### ue-rag（项目语料，独立 server）

调用：`search_ue_project(query="<中文或英文自然语言>", top_k=5)`。
返回：`list[dict]`，每条含 `id / content / metadata / distance`。
`metadata` 里已包含：`source_type`（ue_asset / ue_actor / ue_cpp）、`asset_path`、`class_name`、`display_name`。
**即：一次调用即可同时获得「项目里有什么资产」+「它们在哪」+「它们是什么类」，无需额外列目录了解结构。**

用途：模糊/描述性查询项目内容。**不传确切路径，不传类全名**。
概览：`get_project_summary()` 无参，返回语料统计（collection=ue_assets, total=N）。

命名空间：`mcp_ue_rag`。只提供项目语料检索。工具目录不在这个 server 里，`ue_tools` 只通过上面的扫描脚本访问。

## 求助

工具扫描换一种说法再扫一次仍不相关：**停**，只说一句——“当前任务需要 [动作]，未知 MCP 工具全路径和 JSON 参数。”不要回退全量 schema，不要 `tool_search`。调用成功后的新坑回写 `ue_mcp_verified.json`（只 `rg` 窄查）与对应 Skill。

资产检索为空或不相关：**不算求助**，可以回退文件扫描。不要为此去拉 unreal 全量工具。

## 维护

改 Skill/`AGENTS.md` 直接写文件。YAML 从文件首字节 `---` 开始、无 BOM。索引块由 `apply_index_block.ps1` 生成，勿手改块内。改完跑 `check_skills.ps1`。

域 Skill 正文以 `.codex/skills` 为源；改完必须同步到 `.cursor/skills` 同名文件。`ue-cpp-plan-skill`、`ue-rag-route-skill`、`.cursor/rules`、`.cursor/memory` 只存在于 `.cursor`。
