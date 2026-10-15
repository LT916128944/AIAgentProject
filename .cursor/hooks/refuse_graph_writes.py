"""Cursor beforeShellExecution：拦 call_mcp_tool 里的图写入。"""
import json
import sys
from pathlib import Path

BP_WRITES = {
    "write_graph_dsl",
    "create_node",
    "connect_pins",
    "add_event",
    "add_variable",
    "set_pin_value",
}
_READ_PREFIXES = ("get", "list", "find", "read", "search", "discover")
_ENV = Path(__file__).resolve().parents[3] / "RAG" / "ue-rag" / ".env"
_DENY_BP = "蓝图改图已关闭。把连线步骤写入 Plan/，不要调用写图工具。"
_DOMAIN_KEYS = (
    ("material", "material", "UE_ALLOW_MATERIAL"),
    ("umg", "umg", "UE_ALLOW_UMG"),
    ("niagara", "niagara", "UE_ALLOW_NIAGARA"),
    ("animblueprint", "anim", "UE_ALLOW_ANIM"),
    ("animation", "anim", "UE_ALLOW_ANIM"),
    ("controlrig", "anim", "UE_ALLOW_ANIM"),
)


def env_flag(env_path: Path, key: str) -> bool:
    if not env_path.is_file():
        return False
    try:
        lines = env_path.read_text(encoding="utf-8").splitlines()
    except OSError:
        return False
    for line in lines:
        text = line.strip()
        if not text or text.startswith("#") or "=" not in text:
            continue
        name, value = text.split("=", 1)
        if name.strip() != key:
            continue
        return value.strip().strip('"').strip("'").lower() in {"1", "true", "yes", "on"}
    return False


def blueprint_allowed(env_path: Path | None = None) -> bool:
    return env_flag(env_path if env_path is not None else _ENV, "UE_ALLOW_BLUEPRINT")


def domain_allows(env_path: Path | None = None) -> dict:
    path = env_path if env_path is not None else _ENV
    return {
        "material": env_flag(path, "UE_ALLOW_MATERIAL"),
        "umg": env_flag(path, "UE_ALLOW_UMG"),
        "anim": env_flag(path, "UE_ALLOW_ANIM"),
        "niagara": env_flag(path, "UE_ALLOW_NIAGARA"),
    }


def _flag(command: str, name: str) -> str:
    import re
    match = re.search(rf"--{name}(?:\s+|=)(\"[^\"]+\"|'[^']+'|\S+)", command)
    if not match:
        return ""
    return match.group(1).strip("\"'")


def _is_read(tool: str) -> bool:
    return tool.lower().startswith(_READ_PREFIXES)


def _domain_key(toolset: str) -> str:
    for token, _name, key in _DOMAIN_KEYS:
        if token in toolset:
            return key
    return ""


def decide(command: str, allow_bp: bool, allows: dict | None = None) -> dict:
    if "call_mcp_tool" not in command:
        return {"permission": "allow"}
    tool = _flag(command, "tool")
    toolset = _flag(command, "toolset").lower()
    allows = allows or {}
    domain_key = _domain_key(toolset)
    if domain_key and tool and not _is_read(tool):
        domain = next(name for token, name, key in _DOMAIN_KEYS if key == domain_key and token in toolset)
        if not allows.get(domain):
            text = f"{domain_key} 不是 true。写工具已关闭，步骤留在对话里。"
            return {"permission": "deny", "user_message": text, "agent_message": text}
    elif tool in BP_WRITES and not allow_bp:
        return {"permission": "deny", "user_message": _DENY_BP, "agent_message": _DENY_BP}
    return {"permission": "allow"}


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
