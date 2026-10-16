import json
import os
import sys

out = r"E:\UEProject\AIWork\RAG\ue-rag\contract\query_actor.toolset_schema.json"
note = r"E:\UEProject\AIWork\RAG\ue-rag\contract\schema_export_note.txt"
here = os.path.dirname(__file__)
if here not in sys.path:
    sys.path.append(here)

try:
    import unreal
    from toolset_registry.registration import Registration
    from aiagent_query_actor_toolset import AIAgentQueryActorToolset

    registered = Registration([AIAgentQueryActorToolset]).register()
    try:
        schema = unreal.ToolsetRegistry.get_toolset_json_schema(AIAgentQueryActorToolset)
    except Exception:
        schema = unreal.ToolsetRegistry.get_toolset_json_schema(AIAgentQueryActorToolset.static_class())
    text = schema if isinstance(schema, str) else json.dumps(schema, ensure_ascii=False)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w", encoding="utf-8") as handle:
        handle.write(text)
    message = f"register={registered} bytes={len(text)}"
except Exception as exc:
    message = f"{type(exc).__name__}: {exc}"

with open(note, "w", encoding="utf-8") as handle:
    handle.write(message)
message
