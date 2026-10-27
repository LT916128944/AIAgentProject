# 把 Codex 的技能发现目录联到 .codex/skills。正文只保留一份。
# Codex 读 .agents/skills，不读 .codex/skills。联接已在则直接退出。
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$link = Join-Path $root ".agents\skills"
$target = Join-Path $PSScriptRoot "skills"
if (Test-Path -LiteralPath $link) {
    Write-Output "exists $link"
    exit 0
}
New-Item -ItemType Directory -Path (Join-Path $root ".agents") -Force | Out-Null
cmd /c mklink /J "$link" "$target"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Output "linked $link -> $target"
