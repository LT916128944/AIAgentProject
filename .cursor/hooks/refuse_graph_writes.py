"""Cursor beforeShellExecution：拦 call_mcp_tool 里的图写入。

判定实现已迁到 core.graph_write_policy，Cursor/Codex/DSH 网关三处共用一份。
本文件只保留 Cursor 的 stdin/stdout 协议。"""
import json
import sys
from pathlib import Path

_ROOT = Path(__file__).resolve().parents[3] / "RAG" / "ue-rag"
if str(_ROOT) not in sys.path:
    sys.path.insert(0, str(_ROOT))

from core.graph_write_policy import (  # noqa: E402,F401
    BP_WRITES,
    _DENY_BP,
    _DOMAIN_KEYS,
    _ENV,
    blueprint_allowed,
    decide,
    domain_allows,
    env_flag,
)


def _command(payload: dict) -> str:
    command = payload.get("command")
    if isinstance(command, str):
        return command
    tool_input = payload.get("tool_input") or payload.get("arguments") or {}
    if isinstance(tool_input, dict):
        return str(tool_input.get("command") or "")
    return ""


def main() -> int:
    raw = sys.stdin.read()
    try:
        payload = json.loads(raw) if raw.strip() else {}
    except json.JSONDecodeError:
        payload = {}
    if not isinstance(payload, dict):
        payload = {}
    result = decide(_command(payload), blueprint_allowed(), domain_allows())
    sys.stdout.write(json.dumps(result, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
