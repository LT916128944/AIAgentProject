"""Codex SessionStart：注入与 .cursor/rules 对等的四份短文。"""
import json
import sys
from pathlib import Path

_DIR = Path(__file__).resolve().parents[1] / "instructions"
_FILES = (
    "ue-harness-session.md",
    "ue-rag-route.md",
    "ue-cpp-plan.md",
    "ue-history.md",
)


def additional_context(directory: Path | None = None) -> str:
    root = directory if directory is not None else _DIR
    parts = [
        "以下四份短文在 Codex 里始终有效，对应 Cursor 的 .cursor/rules。技能正文在 .codex/skills。",
    ]
    for name in _FILES:
        path = root / name
        if not path.is_file():
            parts.append(f"（缺 {name}）")
            continue
        parts.append(path.read_text(encoding="utf-8").strip())
    return "\n\n".join(parts)


def main() -> int:
    sys.stdin.read()
    payload = {
        "hookSpecificOutput": {
            "hookEventName": "SessionStart",
            "additionalContext": additional_context(),
        }
    }
    sys.stdout.write(json.dumps(payload, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
