# Self-check every SKILL.md for the failure modes that silently cost tokens:
#   - broken/duplicated YAML frontmatter (loader drops the skill, or the body lies)
#   - frontmatter name not matching the folder name, or missing description
#   - skill missing from apply_index_block.ps1 (it would get a placeholder block)
#   - missing or duplicated UE_MCP_INDEX block, or a block that is not near the top
#   - index version drifting from ue_mcp_index.json
#   - oversized skills (their full body goes into context)
# Output is terse on purpose: only problems + one summary line.
#
#   powershell -ExecutionPolicy Bypass -File .codex\skills\ue-low-token-skill\references\tools\check_skills.ps1

param(
    [string]$SkillsRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..\..')).Path
)

$ErrorActionPreference = 'Stop'

$IndexPath = Join-Path $PSScriptRoot '..\ue_mcp_index.json'
$VerifiedPath = Join-Path $PSScriptRoot '..\ue_mcp_verified.json'
$ApplyPath = Join-Path $PSScriptRoot 'apply_index_block.ps1'
$SizeCap = 4096
$problems = 0
$checked = 0

function Report([string]$skill, [string]$msg) {
    Write-Output ("FAIL {0,-22} {1}" -f $skill, $msg)
    $script:problems++
}

$index = Get-Content -LiteralPath $IndexPath -Raw | ConvertFrom-Json
$applyText = [System.IO.File]::ReadAllText($ApplyPath)
$date = ([string]$index.generated_at).Substring(0, 10)
$engine = (([string]$index.engine_version) -split '-')[0]
$expectedVersion = "索引版本：{0} / engine {1}（{2} 工具集 / {3} 工具）" -f $date, $engine, $index.toolset_count, $index.tool_count

Get-ChildItem -LiteralPath $SkillsRoot -Directory | ForEach-Object {
    $path = Join-Path $_.FullName 'SKILL.md'
    if (-not (Test-Path -LiteralPath $path)) { return }
    $skill = $_.Name
    $script:checked++

    $raw = [System.IO.File]::ReadAllText($path)
    if ($raw.Length -ne 0 -and [int][char]$raw[0] -eq 0xFEFF) { Report $skill 'BOM at byte 0' }
    $lines = $raw -split "`r?`n"

    if ($lines.Count -lt 4 -or $lines[0] -ne '---') { Report $skill 'first line is not ---'; return }
    $fmEnd = -1
    for ($i = 1; $i -lt [Math]::Min(9, $lines.Count); $i++) {
        if ($lines[$i] -eq '---') { $fmEnd = $i; break }
    }
    if ($fmEnd -lt 0) { Report $skill 'no frontmatter terminator within 8 lines'; return }

    $fm = @{}
    $fmLines = if ($fmEnd -gt 1) { $lines[1..($fmEnd - 1)] } else { @() }
    foreach ($line in $fmLines) {
        if ($line -notmatch ':') { continue }
        $k, $v = $line -split ':', 2
        $fm[$k.Trim()] = $v.Trim()
    }
    if (-not $fm['name']) { Report $skill 'frontmatter has no name' }
    elseif ($fm['name'] -ne $skill) { Report $skill ("frontmatter name '{0}' != folder '{1}'" -f $fm['name'], $skill) }
    if (-not $fm['description']) { Report $skill 'frontmatter has no description' }
    if ($skill -ne 'ue-low-token-skill' -and -not $applyText.Contains("'" + $skill + "'")) {
        Report $skill 'not registered in apply_index_block.ps1 $SkillToolsets'
    }

    $begin = -1; $end = -1
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($begin -lt 0 -and $lines[$i].StartsWith('<!-- UE_MCP_INDEX:BEGIN')) { $begin = $i; continue }
        if ($begin -ge 0 -and $end -lt 0 -and $lines[$i].StartsWith('<!-- UE_MCP_INDEX:END')) { $end = $i }
    }
    if ($begin -lt 0) { Report $skill 'no UE_MCP_INDEX:BEGIN'; return }
    if ($end -lt 0) { Report $skill 'no UE_MCP_INDEX:END'; return }
    if ($begin -gt 20) { Report $skill "index block starts at line $($begin + 1) (expected <= 20)" }
    if ($begin -le $fmEnd) { Report $skill 'index block overlaps frontmatter' }

    if ($begin -gt $fmEnd + 1) {
        $extra = @($lines[($fmEnd + 1)..($begin - 1)] | Where-Object { $_ -eq '---' }).Count
        if ($extra -gt 0) { Report $skill "stray '---' x$extra before index block (duplicated frontmatter)" }
    }

    # any '---' line outside the frontmatter and outside fenced code = leftover duplicated frontmatter
    $inFence = $false
    $dashLines = @()
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i].TrimStart().StartsWith('```')) { $inFence = -not $inFence; continue }
        if (-not $inFence -and $lines[$i] -eq '---' -and $i -ne 0 -and $i -ne $fmEnd) { $dashLines += ($i + 1) }
    }
    if ($dashLines.Count -gt 0) { Report $skill ("stray '---' at line(s) {0}" -f ($dashLines -join ',')) }

    $versionLine = $lines[($begin..$end)] | Where-Object { $_ -match '索引版本：' }
    if (-not $versionLine) { Report $skill 'no version line in index block' }
    elseif ($versionLine -notmatch [regex]::Escape($expectedVersion)) { Report $skill 'index version stale vs ue_mcp_index.json' }

    $size = (Get-Item -LiteralPath $path).Length
    if ($size -gt $SizeCap) { Report $skill "size $size > $SizeCap bytes" }
}

try { $null = Get-Content -LiteralPath $VerifiedPath -Raw | ConvertFrom-Json }
catch { Report 'ue_mcp_verified.json' 'invalid JSON' }

Write-Output ("checked={0} skills, problems={1}" -f $checked, $problems)
exit $(if ($problems -gt 0) { 1 } else { 0 })
