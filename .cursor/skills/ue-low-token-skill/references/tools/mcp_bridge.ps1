# Minimal MCP JSON-RPC bridge straight to the Unreal editor's ModelContextProtocol server.
#
# Use this when the agent's tool surface does not expose mcp__unreal_mcp::call_tool
# (or when tool_search returns the schemas but the calls do not actually dispatch).
#
# Endpoint: POST http://127.0.0.1:8000/mcp  (JSON-RPC 2.0, MCP protocol 2025-11-25)
# The port/path come from UModelContextProtocolSettings (ServerPortNumber / ServerUrlPath).
# If the port is not listening, start the server in the editor with the console command
# `ModelContextProtocol.StartServer`.
#
# Examples:
#   powershell -ExecutionPolicy Bypass -File mcp_bridge.ps1 `
#     -ToolsetName editor_toolset.toolsets.scene.SceneTools -ToolName get_current_level
#
#   powershell -ExecutionPolicy Bypass -File mcp_bridge.ps1 `
#     -ToolsetName editor_toolset.toolsets.programmatic.ProgrammaticToolset `
#     -ToolName execute_tool_script -Script .\some_script.py

param(
    [Parameter(Mandatory = $true)][string]$ToolName,
    [string]$ToolsetName,
    [string]$ArgumentsJson = '{}',
    [string]$Script,
    [string]$Url = 'http://127.0.0.1:8000/mcp',
    [string]$ProtocolVersion = '2025-11-25',
    [int]$TimeoutSec = 300
)

$ErrorActionPreference = 'Stop'

if ($Script) {
    if (-not (Test-Path -LiteralPath $Script)) { throw "Script not found: $Script" }
    $scriptText = Get-Content -LiteralPath $Script -Raw
    $ArgumentsJson = (@{ script = $scriptText } | ConvertTo-Json -Depth 5 -Compress)
    [Console]::Error.WriteLine((
        "[mcp_bridge] script_chars={0} script_type={1} json_chars={2} json_type={3}" -f `
        $scriptText.Length, $scriptText.GetType().Name, $ArgumentsJson.Length, $ArgumentsJson.GetType().Name))
    [Console]::Error.WriteLine((
        "[mcp_bridge] json_head=" + $ArgumentsJson.Substring(0, [Math]::Min(100, $ArgumentsJson.Length))))
}

$toolArguments = $ArgumentsJson | ConvertFrom-Json

function Get-HeaderValue($headers, $name) {
    if (-not $headers) { return $null }
    foreach ($key in $headers.Keys) {
        if ($key -ieq $name) {
            $value = $headers[$key]
            if ($value -is [array]) { return [string]$value[0] }
            return [string]$value
        }
    }
    return $null
}

function Send-Rpc {
    param([string]$Method, $Id, $Params, [string]$SessionId, [switch]$Notification)

    $body = [ordered]@{ jsonrpc = '2.0' }
    if (-not $Notification) { $body.id = $Id }
    $body.method = $Method
    if ($null -ne $Params) { $body.params = $Params }

    # Header names match the C++ constants in ModelContextProtocolServer.cpp.
    $headers = @{
        'Accept'               = 'application/json, text/event-stream'
        'Mcp-Protocol-Version' = $ProtocolVersion
    }
    if ($SessionId) { $headers['Mcp-Session-Id'] = $SessionId }

    $payload = $body | ConvertTo-Json -Depth 40 -Compress
    try {
        $response = Invoke-WebRequest -Uri $Url -Method Post -Headers $headers `
            -ContentType 'application/json' -Body $payload -UseBasicParsing -TimeoutSec $TimeoutSec
        return @{ Status = [int]$response.StatusCode; Body = $response.Content; Headers = $response.Headers }
    }
    catch {
        $webResponse = $_.Exception.Response
        if (-not $webResponse) { throw }
        $reader = New-Object System.IO.StreamReader($webResponse.GetResponseStream())
        $content = $reader.ReadToEnd()
        $reader.Dispose()
        return @{ Status = [int]$webResponse.StatusCode; Body = $content; Headers = $webResponse.Headers }
    }
}

$initParams = [ordered]@{
    protocolVersion = $ProtocolVersion
    capabilities    = [ordered]@{}
    clientInfo      = [ordered]@{ name = 'codex-shell-bridge'; version = '1.0' }
}

$init = Send-Rpc -Method 'initialize' -Id 1 -Params $initParams
if ($init.Status -ne 200) {
    Write-Error "initialize failed (HTTP $($init.Status)): $($init.Body)"
    exit 1
}

$sessionId = Get-HeaderValue $init.Headers 'Mcp-Session-Id'
if (-not $sessionId) {
    Write-Error 'initialize returned no Mcp-Session-Id header'
    exit 1
}

Send-Rpc -Method 'notifications/initialized' -SessionId $sessionId -Notification | Out-Null

if ($ToolsetName) {
    $callParams = [ordered]@{
        name      = 'call_tool'
        arguments = [ordered]@{
            toolset_name = $ToolsetName
            tool_name    = $ToolName
            arguments    = $toolArguments
        }
    }
}
else {
    $callParams = [ordered]@{ name = $ToolName; arguments = $toolArguments }
}

$call = Send-Rpc -Method 'tools/call' -Id 2 -Params $callParams -SessionId $sessionId
if ($call.Status -ne 200) {
    Write-Error "tools/call failed (HTTP $($call.Status)): $($call.Body)"
    exit 1
}

$rpc = $call.Body | ConvertFrom-Json
if ($rpc.error) {
    Write-Error "JSON-RPC error $($rpc.error.code): $($rpc.error.message)"
    exit 1
}

$text = ($rpc.result.content | Where-Object { $_.type -eq 'text' } | Select-Object -First 1).text
if ($rpc.result.isError) {
    Write-Error "tool error: $text"
    exit 2
}

Write-Output $text
