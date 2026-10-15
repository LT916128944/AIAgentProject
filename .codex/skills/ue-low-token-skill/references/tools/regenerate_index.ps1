# Regenerate the UE MCP tool index (ue_mcp_index.json).
#
# Run this after an engine or plugin upgrade:
#   powershell -ExecutionPolicy Bypass -File .codex\skills\ue-low-token-skill\references\tools\regenerate_index.ps1
#
# It launches a headless editor, dumps every registered toolset schema, and
# overwrites references/ue_mcp_index.json. No MCP connection is used and the
# schema never passes through an agent's context.

param(
    [string]$EngineRoot = ''
)

$ErrorActionPreference = 'Stop'

$UeRag = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..\..\..\..\..\RAG\ue-rag')).Path
$Py = Join-Path $UeRag '.venv\Scripts\python.exe'
if (-not (Test-Path -LiteralPath $Py)) {
    Write-Error "Required path not found: $Py"
    exit 1
}

function Read-UeConfig([string]$Name) {
    $code = "import sys; sys.path.insert(0, r'$UeRag'); from config import $Name; print($Name)"
    return (& $Py -c $code).Trim()
}

$ProjectFile = Read-UeConfig 'UE_PROJECT'
if (-not $EngineRoot) {
    $EngineRoot = Read-UeConfig 'UE_ENGINE_ROOT'
}
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$DumpScript = Join-Path $PSScriptRoot 'dump_ue_mcp_index.py'
$IndexPath = Join-Path $PSScriptRoot '..\ue_mcp_index.json'

foreach ($required in @($ProjectFile, $EditorCmd, $DumpScript)) {
    if (-not (Test-Path -LiteralPath $required)) {
        Write-Error "Required path not found: $required"
        exit 1
    }
}

$LogPath = Join-Path $env:TEMP ('ue_mcp_index_{0}.log' -f (Get-Date -Format 'yyyyMMdd_HHmmss'))
Write-Host "Dumping toolset schemas (log: $LogPath)"

& $EditorCmd $ProjectFile "-ExecutePythonScript=$DumpScript" `
    -unattended -nosplash -nullrhi -NoSourceControl -stdout *> $LogPath
$ExitCode = $LASTEXITCODE

$Summary = Select-String -LiteralPath $LogPath -Pattern 'UE_MCP_INDEX_RESULT' |
    Select-Object -Last 1 -ExpandProperty Line

if ($Summary) {
    Write-Host $Summary.Trim()
} else {
    Write-Warning "No result marker found. Last editor errors:"
    Select-String -LiteralPath $LogPath -Pattern 'Error:' |
        Select-Object -Last 10 -ExpandProperty Line
}

if (Test-Path -LiteralPath $IndexPath) {
    $info = Get-Item -LiteralPath $IndexPath
    Write-Host ("Index: {0} ({1:N0} bytes, {2})" -f $info.FullName, $info.Length, $info.LastWriteTime)
} else {
    Write-Error "Index file was not written: $IndexPath"
}

exit $ExitCode
