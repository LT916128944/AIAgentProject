"""Codex PreToolUse：拦 shell 里的 call_mcp_tool 写图。

判定实现已迁到 core.graph_write_policy（与 Cursor hook、DSH 网关共用）。"""
import json
import sys
from pathlib import Path

_ROOT = Path(__file__).resolve().parents[3] / "RAG" / "ue-rag"
if str(_ROOT) not in sys.path:
    sys.path.insert(0, str(_ROOT))

from core import graph_write_policy  # noqa: E402


def load_policy():
    return graph_write_policy


def command_from(payload: dict) -> str:
    tool_input = payload.get("tool_input") or payload.get("arguments") or {}
    if isinstance(tool_input, dict):
        command = tool_input.get("command")
        if isinstance(command, str):
            return command
    command = payload.get("command")
    return command if isinstance(command, str) else ""


def codex_result(command: str, allow_bp: bool, allows: dict | None = None) -> dict:
    decision = graph_write_policy.decide(command, allow_bp, allows)
    if decision.get("permission") == "deny":
        reason = decision.get("agent_message") or decision.get("user_message") or "写图已关闭"
        output = {
            "hookEventName": "PreToolUse",
            "permissionDecision": "deny",
            "permissionDecisionReason": reason,
        }
    else:
        output = {
            "hookEventName": "PreToolUse",
            "permissionDecision": "allow",
        }
    return {"hookSpecificOutput": output}


def main() -> int:
    raw = sys.stdin.read()
    try:
        payload = json.loads(raw) if raw.strip() else {}
    except json.JSONDecodeError:
        payload = {}
    if not isinstance(payload, dict):
        payload = {}
    policy = load_policy()
    result = codex_result(command_from(payload), policy.blueprint_allowed(), policy.domain_allows())
    sys.stdout.write(json.dumps(result, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
