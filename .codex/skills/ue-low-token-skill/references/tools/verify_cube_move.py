"""Move the actor named 'Cube' back to world X = 1000, then read it back.

Runs inside the ProgrammaticToolset sandbox:
  execute_tool_script(script=<this file>)
Only the allow-listed stdlib modules are importable here, and `execute_tool`
is injected as a builtin.
"""

import json

SCENE = 'editor_toolset.toolsets.scene.SceneTools'
ACTOR = 'editor_toolset.toolsets.actor.ActorTools'
TARGET_X = 1000.0


def _vec(node):
    """Return the first {x, y, z} dict found anywhere in the value."""
    if isinstance(node, dict):
        if all(k in node for k in ('x', 'y', 'z')):
            return node
        for value in node.values():
            found = _vec(value)
            if found:
                return found
    elif isinstance(node, list):
        for value in node:
            found = _vec(value)
            if found:
                return found
    return None


def run():
    out = {}

    found = execute_tool(
        SCENE + '.find_actors',
        json.dumps({'name': 'Cube', 'tag': '', 'collision_channels': []}),
    )
    actors = found.get('returnValue') if isinstance(found, dict) else None
    if not isinstance(actors, list) or not actors:
        out['status'] = 'no_cube'
        out['find_raw'] = json.dumps(found)[:400]
        return out

    actor = actors[0]
    out['count'] = len(actors)
    out['actor'] = json.dumps(actor)[:300]

    before = execute_tool(ACTOR + '.get_actor_transform', json.dumps({'actor': actor}))
    out['before'] = json.dumps(before)[:400]

    current = _vec(before)
    if not current:
        out['status'] = 'no_vector'
        return out

    xform = {'location': {'x': TARGET_X, 'y': current['y'], 'z': current['z']}}
    out['sent'] = json.dumps(xform)

    execute_tool(
        ACTOR + '.set_actor_transform',
        json.dumps({'actor': actor, 'xform': xform, 'worldspace': True}),
    )

    after = execute_tool(ACTOR + '.get_actor_transform', json.dumps({'actor': actor}))
    out['after'] = json.dumps(after)[:400]
    out['status'] = 'ok'
    return out
