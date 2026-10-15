# 外部技能包评审：UnrealEngine5-Skills-main（2026-09-21）

来源：本仓库 `UnrealEngine5-Skills-main/`（MIT，宣称支持 UE5.6–5.8，11 个 Skill + 2 个校验脚本 + 模块索引 CSV）。
结论一句话：**流程纪律可吸收，工具层不可照抄。**

本文件是评审记录，不随会话自动加载；运行事实一律以 `ue_mcp_verified.json` 为准。

## 一、它是什么

- 纯提示词/工作流包。每个 Skill 的结构固定：frontmatter → Quick Start → API Anchors → Stage Contract → Workflow → Constraints → Failure Handling → Escalation → references/。
- 它引用的 MCP 工具（`blueprint_modify`、`spawn_actor`、`get_output_log`、`execute_script`…）指向**另一套 MCP 服务器**；本机 `unreal_mcp` 里没有这些工具名，一律不可照抄。
- 没有任何参数级事实、没有踩坑记录、没有 token 预算纪律；11 个正文合计约 50KB，全量加载不现实。

## 二、11 个 Skill 与我们的对应关系

| 他们的 Skill | 覆盖内容 | 我们的对应 | 可吸收点 |
| --- | --- | --- | --- |
| ue5-auto-assistant | 自然语言意图路由 | `ue-low-token-skill` 路由规则 | 路由带置信度、只澄清一次 |
| ue5-module-router | 按引擎模块名路由 | 无（我们是工具集路由） | 精确匹配优先于关键词猜测 |
| ue5-architecture | 模块/Build.cs 边界 | 无 | 分层方向规则（内容知识，非 MCP） |
| ue5-blueprint-workflow | 蓝图图编辑/输入事件 | `ue-blueprint-skill` | 阶段契约、入口事件优先、exec 先于 data |
| ue5-cpp-gameplay | Gameplay C++ | 无 | `.h/.cpp` 成对产出、反射清单 |
| ue5-debug-validation | 排障与验证 | **新建 `ue-debug-skill`** | 症状 → 定位 → 修复；先复现再改 |
| ue5-performance-packaging | 性能与打包 | 无 | 无度量不下结论；先解第一个阻塞错误 |
| ue5-save-load-replication | 存档与同步 | 无 | 状态三分类：只持久 / 只复制 / 两者 |
| ue5-ui-umg-slate | UMG / Slate | `ue-umg-skill` | 焦点归还、销毁清理、显示与状态分离 |
| ue5-world-interaction | 拾取/生成器/交互 | `ue-actor-skill` 部分 | 检测 → 资格 → 结果 → 反馈 → 生命周期 |
| ue5-pcg-building | PCG 建筑生成 | `ue-pcg-skill` | 六阶段流水线 + 显式 Seed 确定性 |

## 三、本次吸收（已落盘）

1. **阶段契约**：动手前定齐必要项，缺一即停。已加到 actor / blueprint / asset / object / material / niagara / pcg / sequencer / static-mesh / umg / animation / verify。
2. **症状 → 原因 → 修复 表**：数据只取 `ue_mcp_verified.json` 里的实测事实（如 DSL 根节点限制、`find_node_types` 大小写、`connect_pins` 顶掉连线、PIE 锁资产、`save_actor` 报错、脚本返回值被顶掉）。
3. **工具优先级**：专有工具 > `execute_tool_script` 编排 > `mcp_bridge.ps1`；脚本不得顶替专有工具（`ue-low-token-skill`、`ue-programmatic-skill`）。
4. **新 Skill `ue-debug-skill`**：报错/日志/不生效类请求的专属入口（LogsToolset / ConfigSettings / 蓝图结构取证），诊断阶段只读。
5. **反模式块**：每个域 Skill 补上「不做全量枚举、不做破坏性改动、模糊需求先问清」。
6. **自检增强**：`check_skills.ps1` 现在额外校验 frontmatter `name` == 目录名、`description` 非空、是否登记在 `apply_index_block.ps1` 的 `$SkillToolsets`。
7. **领域经验库**（2026-09-21 用户裁决：外部包 500+ star，按有效采纳、无需本机验证）：`references/ext_ue5_knowledge.md` 收录其全部可复用经验——通用工作法、调试六步法、蓝图链路模式、UMG/Slate 桥接与 tooltip、世界交互五段、PCG 六阶段+预算+验收场景、C++ 反射/复制清单、存档状态三分类、性能打包契约、架构分层与模块路由。按 `## ` 标题窄查，不进常驻上下文。

## 四、刻意不吸收

- **API Anchors 不再进 Skill 正文**：引擎类/函数名（如 `UPCGSubsystem::RefreshRuntimeGenExecutionSources`）统一收进 `references/ext_ue5_knowledge.md` 的对应章节，按需窄查即可；Skill 正文只保留本机实测的工具路径与参数，避免把「外部知识」与「本机已验证事实」混为一谈。
- **145KB 模块索引/路由 CSV**：与「禁全量」铁律冲突。
- **他们的工具名与 `execute_script` 兜底**：工具名不对齐；我们的脚本工具是 `execute_tool_script` 且限制更严。
- **中英双语 README 与截图**：我们不发布文档，纯成本。

## 五、路由自测用例（改完 Skill 后回归用）

| 输入 | 期望路由 | 首个动作 |
| --- | --- | --- |
| 把 Cube 移到 (100,200,300)，不用保存 | `ue-actor-skill` | `find_actors` → `set_actor_transform` → 回读 transform 一次 |
| PIE 里左键开火没反应，帮我查 | `ue-debug-skill` | `GetLogEntries(pattern=..., category="")` 或 `list_events` |
| 帮我把这个材质实例的 Roughness 改成 0.3 | `ue-material-skill` | `set_scalar_parameter` → 回读一次 |
| 给这个 Widget 加个按钮并绑事件 | `ue-umg-skill` | `AddWidget` → `BindToEventProperty` → `CompileWidgetBlueprint` |
| PCG 生成的建筑每次都不同 | `ue-pcg-skill` | 先查 Seed 是否显式绑定 |
| 保存时报 is not an external actor asset | `ue-debug-skill` | `get_current_level` → `save_assets([level])` |

## 六、维护约定

- 外部包内容值得长期保留的只有本文件里的要点；`UnrealEngine5-Skills-main/` 目录可保留作对照，不纳入 `.codex/skills`。
- 任何从外部包搬来的结论，必须先在本机 MCP 上跑通，再回写 `ue_mcp_verified.json` 与对应 Skill。
