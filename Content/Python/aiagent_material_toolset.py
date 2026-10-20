# Content/Python/aiagent_material_toolset.py
"""项目自己的材质资产 Toolset。只标一个 AICallable。

2026-10-20 阶段 A 为验收扇补唯一缺口：引擎侧没有任何 MCP 工具能创建 Material 资产
（BlueprintTools.create 对 /Script/Engine.Material 会RaiseScriptError 后断连），
材质图连线按约定走人工，但侑明确授权用脚本一次性建一个单标量参数的 M_Coin。
幂等：资产已存在就直接返回，不重复建，不重复连线。
"""
import unreal
import toolset_registry


@unreal.uclass()
class AIAgentMaterialToolset(unreal.ToolsetDefinition):
    """创建带单个标量参数的材质资产，参数连到 Emissive Color。"""

    @toolset_registry.tool_call
    @staticmethod
    def create_material_with_param(asset_name: str, parameter_name: str) -> str:
        """在 /Game 下建一个只含 Emissive 类标量参数的材质，保存返回。

        Args:
            asset_name: 材质资产名（如 M_Coin）。
            parameter_name: 要暴露的标量参数名（如 Emissive）。

        Returns:
            object_path 文本，或 fail= 开头的错误原文。
        """
        asset_name = (asset_name or "").strip()
        parameter_name = (parameter_name or "").strip()
        if not asset_name or not parameter_name:
            return "fail 参数为空"
        object_path = f"/Game/{asset_name}.{asset_name}"
        try:
            if unreal.EditorAssetLibrary.does_asset_exist(f"/Game/{asset_name}"):
                return f"exists {object_path}"
            asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
            material = asset_tools.create_asset(asset_name, "/Game", unreal.Material, unreal.MaterialFactoryNew())
            if not material:
                return "fail create_asset 返回空"
            expression = unreal.MaterialEditingLibrary.create_material_expression(
                material, unreal.MaterialExpressionScalarParameter, -300.0, 0.0)
            if not expression:
                return "fail create_material_expression 返回空"
            expression.set_editor_property("parameter_name", unreal.Name(parameter_name))
            expression.set_editor_property("default_value", 1.0)
            unreal.MaterialEditingLibrary.connect_material_property(
                expression, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
            unreal.MaterialEditingLibrary.recompile_material(material)
            unreal.EditorAssetLibrary.save_asset(f"/Game/{asset_name}")
            return f"ok {object_path}"
        except Exception as exc:  # 单步失败带回原文
            return f"fail {type(exc).__name__}: {exc}"