"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>."""

import unreal


def create_material():
    path = "/Game/Powerups/M_Pulso"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Pulso", "/Game/Powerups", unreal.Material, unreal.MaterialFactoryNew()
    )
    if not material:
        raise RuntimeError("Could not create M_Pulso.")
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("disable_depth_test", True)
    material.set_editor_property("two_sided", True)
    unreal.MaterialEditingLibrary.set_base_material_usage(
        material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH
    )
    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -320, -100
    )
    color.set_editor_property("constant", unreal.LinearColor(0.05, 2.0, 5.0, 1.0))
    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -320, 100
    )
    opacity.set_editor_property("r", 0.7)
    if not unreal.MaterialEditingLibrary.connect_material_property(
        color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    ):
        raise RuntimeError("Could not connect pulse emissive color.")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        opacity, "", unreal.MaterialProperty.MP_OPACITY
    ):
        raise RuntimeError("Could not connect pulse opacity.")
    errors = unreal.MaterialEditingLibrary.recompile_material(material)
    if errors:
        raise RuntimeError("Pulse material shader errors: " + "; ".join(errors))
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError("Could not save M_Pulso.")
    return material


material = create_material()
if material.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_TRANSLUCENT:
    raise RuntimeError("Pulse material must be translucent.")
if not material.get_editor_property("disable_depth_test"):
    raise RuntimeError("Pulse material must render through opaque surfaces.")
if not unreal.MaterialEditingLibrary.has_material_usage(
    material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH
):
    raise RuntimeError("Pulse material must support skeletal meshes.")
unreal.log("DVL_POWERUP_MATERIAL_OK: persistent through-wall skeletal material validated.")

parent = unreal.load_class(None, "/Script/ProyectoDVL.PuntoPowerup")
if not parent:
    raise RuntimeError("Build ProyectoDVLEditor with PuntoPowerup before creating its Blueprints.")

for name, kind in (
    ("BP_Powerup_Pulso", unreal.PowerupDVL.PULSO),
    ("BP_Powerup_Adrenalina", unreal.PowerupDVL.ADRENALINA),
):
    path = "/Game/Powerups/" + name
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, "/Game/Powerups", unreal.Blueprint, factory
        )
        if not blueprint:
            raise RuntimeError("Could not create " + name)
        defaults = unreal.get_default_object(blueprint.generated_class())
        defaults.set_editor_property("tipo", kind)
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint):
            raise RuntimeError("Could not save " + name)
    generated = unreal.EditorAssetLibrary.load_blueprint_class(path)
    if not generated or unreal.get_default_object(generated).get_editor_property("tipo") != kind:
        raise RuntimeError("Unexpected Blueprint type in " + path + "; not overwriting it.")
    unreal.log("DVL_POWERUP_BLUEPRINT_OK: " + path)
