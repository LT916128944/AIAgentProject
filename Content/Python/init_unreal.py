import os
import sys

import unreal
from toolset_registry.registration import Registration

_here = os.path.dirname(__file__)
if _here not in sys.path:
    sys.path.append(_here)

from aiagent_query_actor_toolset import AIAgentQueryActorToolset
from aiagent_material_toolset import AIAgentMaterialToolset

_registration = Registration([AIAgentQueryActorToolset, AIAgentMaterialToolset])
if _registration.register():
    unreal.log("AIAgentQueryActorToolset / AIAgentMaterialToolset registered")
else:
    unreal.log_error("AIAgentQueryActorToolset / AIAgentMaterialToolset register failed")
