# Content/Python/aiagent_query_actor_toolset.py
"""项目自己的读场景 Toolset。只标一个 AICallable。"""
import unreal
import toolset_registry


@unreal.uclass()
class AIAgentQueryActorToolset(unreal.ToolsetDefinition):
    """按名字读当前关卡里唯一的 Actor。"""

    @toolset_registry.tool_call
    @staticmethod
    def query_actor(name: str) -> str:
        """名字唯一时返回位置。零个或多个就停。

        Args:
            name: Actor 标签。

        Returns:
            位置文本，或停和候选。
        """
        subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        matches = [actor for actor in subsystem.get_all_level_actors() if actor.get_actor_label() == name]
        if len(matches) != 1:
            labels = ",".join(actor.get_actor_label() for actor in matches[:8]) or "(空)"
            return f"停 候选={labels}"
        actor = matches[0]
        loc = actor.get_actor_location()
        return f"name={name} ref={actor.get_path_name()} x={loc.x:.1f} y={loc.y:.1f} z={loc.z:.1f}"
