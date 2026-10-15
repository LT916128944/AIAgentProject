"""Dump the full UE MCP toolset index to disk from inside Unreal Editor.

Run with:
  UnrealEditor-Cmd.exe <project>.uproject -ExecutePythonScript=<this file> ^
      -unattended -nosplash -nullrhi

The script never prints the schema itself; it only prints a one-line summary.
Full schema lands in:
  <project>/.codex/skills/ue-low-token-skill/references/ue_mcp_index.json
"""

import datetime
import json
import os

import unreal

INDEX_RELATIVE_PATH = ".codex/skills/ue-low-token-skill/references/ue_mcp_index.json"
SCHEMA_API = "unreal.ToolsetRegistry.get_all_toolset_json_schemas()"


def _tool_bare_name(full_name, toolset_name):
    """Strip the toolset prefix from a fully qualified tool name."""
    prefix = toolset_name + "."
    if full_name.startswith(prefix):
        return full_name[len(prefix):]
    return full_name.rsplit(".", 1)[-1]


def _compact_properties(input_schema):
    """Reduce a JSON schema's properties to the fields an agent actually needs."""
    props = {}
    for key, value in (input_schema.get("properties") or {}).items():
        if not isinstance(value, dict):
            continue
        entry = {"type": value.get("type", "object")}
        for field in ("description", "default", "$ref", "enum", "items"):
            if field in value:
                entry[field] = value[field]
        props[key] = entry
    return props


def build_index():
    """Build the index dict from the live toolset registry."""
    raw = unreal.ToolsetRegistry.get_all_toolset_json_schemas()
    toolsets = json.loads(raw) if raw else []

    index = {
        "generated_at": datetime.datetime.now().replace(microsecond=0).isoformat(),
        "engine_version": unreal.SystemLibrary.get_engine_version(),
        "project": unreal.Paths.get_project_file_path(),
        "schema_api": SCHEMA_API,
        "toolset_count": 0,
        "tool_count": 0,
        "toolsets": {},
    }

    for toolset in toolsets:
        name = toolset.get("name", "")
        if not name:
            continue

        tools = {}
        for tool in toolset.get("tools", []):
            full_name = tool.get("name", "")
            if not full_name:
                continue
            schema = tool.get("inputSchema") or {}
            tools[_tool_bare_name(full_name, name)] = {
                "full_name": full_name,
                "description": tool.get("description", ""),
                "required": schema.get("required", []),
                "properties": _compact_properties(schema),
                "input_schema": schema,
            }

        index["toolsets"][name] = {
            "description": toolset.get("description", ""),
            "version": toolset.get("version", ""),
            "tool_count": len(tools),
            "tools": tools,
        }
        index["toolset_count"] += 1
        index["tool_count"] += len(tools)

    return index


def main():
    if not unreal.ToolsetRegistry.is_available():
        print("UE_MCP_INDEX_RESULT status=registry_unavailable")
        return

    index = build_index()
    out_path = os.path.join(
        unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()),
        INDEX_RELATIVE_PATH.replace("/", os.sep),
    )
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as handle:
        json.dump(index, handle, indent=1, ensure_ascii=False)

    print(
        "UE_MCP_INDEX_RESULT status=ok toolsets={} tools={} bytes={} path={}".format(
            index["toolset_count"],
            index["tool_count"],
            os.path.getsize(out_path),
            out_path,
        )
    )


main()
