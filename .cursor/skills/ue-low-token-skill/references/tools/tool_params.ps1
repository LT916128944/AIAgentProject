# Print the parameter skeleton of one UE MCP tool: required args + property names/types only.
# Descriptions are deliberately dropped: they dominate the index and cost tokens for no decision value.
#
#   tool_params.ps1 find_actors          # one tool -> required + properties
#   tool_params.ps1 -Search attach       # find tool names across all toolsets (max 40 lines)
#   tool_params.ps1 find_actors -Detail  # add a 200-char description

param(
    [Parameter(Position = 0)][string]$Tool,
    [string]$Search,
    [switch]$Detail
)

$ErrorActionPreference = 'Stop'
$IndexPath = Join-Path $PSScriptRoot '..\ue_mcp_index.json'
if (-not (Test-Path -LiteralPath $IndexPath)) {
    Write-Error "Index not found: $IndexPath (run regenerate_index.ps1)"
}

$index = Get-Content -LiteralPath $IndexPath -Raw | ConvertFrom-Json
$toolsets = $index.toolsets.PSObject.Properties

if ($Search) {
    $hits = 0
    foreach ($ts in $toolsets) {
        foreach ($t in $ts.Value.tools.PSObject.Properties) {
            if ($t.Name -like "*$Search*") {
                $req = if ($t.Value.required) { $t.Value.required -join ',' } else { '' }
                Write-Output ("{0} :: {1} [{2}]" -f $ts.Name, $t.Name, $req)
                $hits++
                if ($hits -ge 40) { Write-Output '...(truncated at 40; narrow the keyword)'; break }
            }
        }
        if ($hits -ge 40) { break }
    }
    if ($hits -eq 0) { Write-Output "no tool matching '$Search'"; exit 1 }
    exit 0
}

if (-not $Tool) {
    Write-Output 'usage: tool_params.ps1 <tool_name> [-Detail] | -Search <keyword>'
    exit 2
}

$found = 0
foreach ($ts in $toolsets) {
    foreach ($t in $ts.Value.tools.PSObject.Properties) {
        $full = $t.Value.full_name
        $match = ($t.Name -eq $Tool) -or ($full -eq $Tool)
        if ($full -and $full.EndsWith(".$Tool")) { $match = $true }
        if (-not $match) { continue }

        $found++
        $required = @($t.Value.required)
        $props = $t.Value.properties.PSObject.Properties
        Write-Output ("toolset : {0}" -f $ts.Name)
        Write-Output ("tool    : {0}" -f $t.Name)
        $propLines = foreach ($p in $props) {
            $mark = if ($required -contains $p.Name) { '*' } else { ' ' }
            "{0}{1}:{2}" -f $mark, $p.Name, $p.Value.type
        }
        Write-Output ("required: {0}" -f (($required | Where-Object { $_ }) -join ', '))
        Write-Output ("props   : {0}" -f (($propLines -join ', ')))
        if ($Detail) {
            $d = [string]$t.Value.description
            if ($d.Length -gt 200) { $d = $d.Substring(0, 200) + '...' }
            Write-Output ("desc    : {0}" -f ($d -replace '\s+', ' '))
        }
    }
}

if ($found -eq 0) {
    Write-Output "tool '$Tool' not found; try -Search <keyword>"
    exit 1
}
