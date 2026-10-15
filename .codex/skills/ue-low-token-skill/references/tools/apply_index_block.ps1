# Insert (or refresh) the authoritative UE MCP index routing block in every SKILL.md.
#
# Idempotent: the block sits between the UE_MCP_INDEX markers and is replaced on
# every run, so the rest of each SKILL.md body is never touched.
#
# Run:
#   powershell -ExecutionPolicy Bypass -File .codex\skills\ue-low-token-skill\references\tools\apply_index_block.ps1

param(
    [string]$SkillsRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..\..')).Path
)

$ErrorActionPreference = 'Stop'

$IndexRel = '.codex/skills/ue-low-token-skill/references/ue_mcp_index.json'
$VerifiedRel = '.codex/skills/ue-low-token-skill/references/ue_mcp_verified.json'
$RegenRel = '.codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1'
$BeginMarker = '<!-- UE_MCP_INDEX:BEGIN'
$EndMarker = '<!-- UE_MCP_INDEX:END -->'

$Template = @'
<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
__TOOLSETS__
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：__VERSION__
<!-- UE_MCP_INDEX:END -->
'@

$SkillToolsets = @{
    'ue-actor-skill'        = @('editor_toolset.toolsets.actor.ActorTools (17)', 'editor_toolset.toolsets.scene.SceneTools (20)', 'editor_toolset.toolsets.primitive.PrimitiveTools (4)')
    'ue-animation-skill'    = @('animation_toolset.toolsets.controlrig.ControlRigTools (44)', 'animation_toolset.toolsets.controlrig_sequencer.SequencerControlRigTools (72)', 'editor_toolset.toolsets.skeletal_mesh.SkeletalMeshTools (22)')
    'ue-asset-skill'        = @('editor_toolset.toolsets.asset.AssetTools (21)', 'editor_toolset.toolsets.data_asset.DataAssetTools (1)', 'editor_toolset.toolsets.data_table.DataTableTools (10)', 'editor_toolset.toolsets.curve_table.CurveTableTools (9)', 'editor_toolset.toolsets.string_table.StringTableTools (8)', 'editor_toolset.toolsets.texture.TextureTools (2)')
    'ue-blueprint-skill'    = @('editor_toolset.toolsets.blueprint.BlueprintTools (53)')
    'ue-debug-skill'        = @('EditorToolset.LogsToolset (4)', 'ConfigSettingsToolset.ConfigSettingsToolset (8)', 'SlateInspectorToolset.SlateInspectorToolset (14)')
    'ue-material-skill'     = @('editor_toolset.toolsets.material.MaterialTools (22)', 'editor_toolset.toolsets.material_instance.MaterialInstanceTools (13)')
    'ue-niagara-skill'      = @('NiagaraToolsets.NiagaraToolset_System (46)', 'NiagaraToolsets.NiagaraToolset_Component (4)', 'NiagaraToolsets.NiagaraToolset_Assets (3)', 'NiagaraToolsets.NiagaraToolset_Blueprint (2)', 'NiagaraToolsets.NiagaraToolset_Info (1)')
    'ue-object-skill'       = @('editor_toolset.toolsets.object.ObjectTools (6)')
    'ue-pcg-skill'          = @('PCGToolset.PCGToolset (30)', 'PCGToolset.PCGSpatialToolset (1)')
    'ue-programmatic-skill' = @('editor_toolset.toolsets.programmatic.ProgrammaticToolset (2)')
    'ue-sequencer-skill'    = @('animation_toolset.toolsets.sequencer.SequencerTools (140)', 'animation_toolset.toolsets.keyframing.SequencerKeyframingTools (22)', 'animation_toolset.toolsets.outliner.SequencerOutlinerTools (18)', 'animation_toolset.toolsets.conditions.SequencerConditionTools (9)', 'animation_toolset.toolsets.custom_bindings.SequencerCustomBindingTools (8)', 'animation_toolset.toolsets.import_export.SequencerImportExportTools (6)')
    'ue-static-mesh-skill'  = @('editor_toolset.toolsets.static_mesh.StaticMeshTools (16)')
    'ue-umg-skill'          = @('UMGToolSet.UMGToolSet (23)')
    'ue-verify-skill'       = @('editor_toolset.toolsets.scene.SceneTools (20)', 'editor_toolset.toolsets.actor.ActorTools (17)', 'editor_toolset.toolsets.programmatic.ProgrammaticToolset (2)')
}

$LowTokenToolsets = @(
    '未见专属 Skill 覆盖的工具集（按需直接查索引）：',
    'ToolsetRegistry.AgentSkillToolset / AutomationTestToolset.AutomationTestToolset / DataflowAgent.DataflowAgentToolset',
    'DataRegistryToolset.DataRegistryTools / EditorToolset.EditorAppToolset / GameplayTagsToolset.GameplayTagsToolset / GameFeaturesToolset.GameFeaturesToolset',
    'GASToolsets.GameplayCueToolset / GASToolsets.AttributeSetToolset / GASToolsets.AbilitySystemInspectorToolset',
    'PhysicsToolsets.PhysicsAssetToolset / PluginToolset.PluginToolset / SemanticSearchToolset.SemanticSearchToolset',
    'WorldConditionsToolset.WorldConditionTools / state_tree_toolset.toolsets.state_tree.StateTreeTools',
    'conversation_toolset.toolsets.conversation.ConversationTools / aimodule_toolset.toolsets.behavior_tree.BehaviorTreeTools',
    '',
    '禁 list_toolsets / describe_toolset，除非本清单/索引都没有。'
)

$IndexMeta = Get-Content -LiteralPath (Join-Path $PSScriptRoot '..\ue_mcp_index.json') -Raw | ConvertFrom-Json
$VersionLine = '{0} / engine {1}（{2} 工具集 / {3} 工具）' -f `
    ([string]$IndexMeta.generated_at).Substring(0, 10), `
    (([string]$IndexMeta.engine_version) -split '-')[0], `
    $IndexMeta.toolset_count, `
    $IndexMeta.tool_count

$skills = Get-ChildItem -LiteralPath $SkillsRoot -Directory |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'SKILL.md') }

foreach ($skill in $skills) {
    $name = $skill.Name
    $path = Join-Path $skill.FullName 'SKILL.md'

    if ($name -eq 'ue-low-token-skill') {
        $body = ($LowTokenToolsets | ForEach-Object { $_ }) -join "`r`n"
    } elseif ($SkillToolsets.ContainsKey($name)) {
        $body = ($SkillToolsets[$name] | ForEach-Object { '- ' + $_ }) -join "`r`n"
    } else {
        Write-Warning "No toolset mapping for $name; writing placeholder."
        $body = '- (未登记；直接查 ' + $IndexRel + ' 的 toolsets 顶层键)'
    }

    $block = $Template.Replace('__TOOLSETS__', $body).Replace('__VERSION__', $VersionLine)

    $raw = [System.IO.File]::ReadAllText($path)
    $newline = if ($raw.Contains("`r`n")) { "`r`n" } else { "`n" }
    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]]($raw -split "`r?`n"))

    $beginIdx = -1
    $endIdx = -1
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($beginIdx -lt 0 -and $lines[$i].StartsWith($BeginMarker)) {
            $beginIdx = $i
            continue
        }
        if ($beginIdx -ge 0 -and $lines[$i].StartsWith($EndMarker)) {
            $endIdx = $i
            break
        }
    }

    $action = 'inserted'
    if ($beginIdx -ge 0 -and $endIdx -gt $beginIdx) {
        $lines.RemoveRange($beginIdx, $endIdx - $beginIdx + 1)
        $action = 'refreshed'
    }

    $frontmatterEnd = -1
    for ($i = 1; $i -lt [Math]::Min(9, $lines.Count); $i++) {
        if ($lines[$i].Trim() -eq '---') { $frontmatterEnd = $i; break }
    }
    if ($frontmatterEnd -lt 0) {
        Write-Warning "$name : could not locate frontmatter end; skipped."
        continue
    }

    $blockLines = [string[]]($block -split "`r?`n")
    $lines.InsertRange($frontmatterEnd + 1, [string[]](@('') + $blockLines))

    $strayDashes = @($lines | Where-Object { $_ -eq '---' }).Count - 2
    if ($strayDashes -gt 0) { Write-Warning ("{0} : stray '---' x{1} (leftover duplicated frontmatter?)" -f $name, $strayDashes) }

    [System.IO.File]::WriteAllText($path, ($lines -join $newline), (New-Object System.Text.UTF8Encoding($false)))

    $head = [System.IO.File]::ReadAllText($path)
    $firstLine = ($head -split "`r?`n")[0]
    $frontmatterOk = if ($firstLine -eq '---') { 'frontmatter-ok' } else { 'FRONTMATTER-BROKEN' }
    Write-Host ("{0,-22} {1,-10} {2}" -f $name, $action, $frontmatterOk)
}
