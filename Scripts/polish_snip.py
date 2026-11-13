"""Save SNIP-only presentation changes in the open editor (outside PIE)."""
import unreal
lib=unreal.MaterialEditingLibrary
coral=unreal.load_asset('/Game/SNIP/Materials/M_Coral')
coral.set_editor_property('used_with_skeletal_mesh',True)
lib.recompile_material(coral)
unreal.EditorAssetLibrary.save_loaded_asset(coral)

# A printed mint arch gives the protected exit an unmistakable doorway silhouette.
m=unreal.load_asset('/Game/SNIP/Materials/M_Exit')
lib.delete_all_material_expressions(m)
uv=lib.create_material_expression(m,unreal.MaterialExpressionTextureCoordinate,-400,0)
c=lib.create_material_expression(m,unreal.MaterialExpressionCustom,-150,0)
inp=unreal.CustomInput();inp.set_editor_property('input_name','UV')
c.set_editor_property('inputs',[inp]);c.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
c.set_editor_property('code','''
float2 p=float2((UV.x-.5)*2,UV.y);
float opening=step(abs(p.x),.69)*step(.16,p.y)*step(p.y,.91);
float knob=1-smoothstep(.045,.06,length((UV-float2(.68,.62))*float2(1,1.8)));
return lerp(lerp(float3(.12,.6,.37),float3(.025,.11,.10),opening),float3(.95,.76,.3),knob);
''')
lib.connect_material_expressions(uv,'',c,'UV')
lib.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
rough=lib.create_material_expression(m,unreal.MaterialExpressionConstant,-150,200);rough.set_editor_property('r',.75)
lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
lib.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)

actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
# An unlit, two-sided paper cyclorama keeps Front and Side view backgrounds
# readable; it is decoration and never participates in puzzle collision/cuts.
bg=unreal.load_asset('/Game/SNIP/Materials/M_Cyclorama')
if not bg:
    bg=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_Cyclorama','/Game/SNIP/Materials',unreal.Material,unreal.MaterialFactoryNew())
    bg.set_editor_property('two_sided',True)
    bg.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    tint=lib.create_material_expression(bg,unreal.MaterialExpressionVectorParameter,-200,0)
    tint.set_editor_property('parameter_name','Tint')
    tint.set_editor_property('default_value',unreal.LinearColor(.58,.56,.48,1))
    lib.connect_material_property(tint,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(bg)
    unreal.EditorAssetLibrary.save_loaded_asset(bg)
back=next((a for a in actors.get_all_level_actors() if a.get_actor_label()=='SNIP / Paper Cyclorama'),None)
if not back:
    back=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(1000,650,500))
    back.set_actor_label('SNIP / Paper Cyclorama')
back.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Sphere'))
back.static_mesh_component.set_material(0,bg)
back.static_mesh_component.set_collision_profile_name('NoCollision')
back.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
back.set_actor_enable_collision(False)
back.tags=['SNIPDecoration']
back.static_mesh_component.set_editor_property('cast_shadow',False)
back.set_actor_scale3d(unreal.Vector(240,240,240))
for a in actors.get_all_level_actors():
    if a.get_class().get_name()=='SNIPPuzzle':
        a.set_editor_property('stage',0);a.build_puzzle()
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('SNIP_POLISH_SAVED')
