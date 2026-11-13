"""Add isolated cinematic prototype materials. Never rewrite existing art assets."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
scope={}
exec(compile((root/'Scripts/build_snip.py').read_text().split("paper = material(")[0],'material_helpers','exec'),scope)
scope['ROOT']='/Game/SNIP/Cinematic'
unreal.EditorAssetLibrary.make_directory('/Game/SNIP/Cinematic/Materials')
make=scope['material']
paper=make('M_SNIP_Paper',(.949,.894,.784),paper=True)
make('M_SNIP_Cardboard',(.851,.471,.259),paper=True)
make('M_SNIP_CardboardEdge',(.349,.271,.212),paper=True)
metal=make('M_SNIP_Metal',(.663,.675,.667),.32,.85)
for name,color in [('M_SNIP_Ghost',(.337,.72,.75)),('M_SNIP_CutSelection',(.839,.424,.396)),('M_SNIP_InvalidCut',(.85,.12,.10))]:
    path='/Game/SNIP/Cinematic/Materials/'+name
    existed=unreal.EditorAssetLibrary.does_asset_exist(path)
    mat=make(name,color)
    if not existed:
        mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
        opacity=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionScalarParameter,-200,500)
        opacity.set_editor_property('parameter_name','Opacity')
        opacity.set_editor_property('default_value',.24)
        unreal.MaterialEditingLibrary.connect_material_property(opacity,'',unreal.MaterialProperty.MP_OPACITY)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat)
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name()=='SNIPPuzzle':
        if not actor.get_editor_property('paper_material'):actor.set_editor_property('paper_material',paper)
        if not actor.get_editor_property('metal_material'):actor.set_editor_property('metal_material',metal)
        actor.build_puzzle()
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('SNIP_CINEMATIC_INTEGRATED')
