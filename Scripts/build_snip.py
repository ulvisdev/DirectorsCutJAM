"""Build the integrated SNIP editor map. Run with Unreal's Python interpreter."""
import unreal

ROOT = '/Game/SNIP'
assets = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.MaterialEditingLibrary
unreal.EditorAssetLibrary.make_directory(ROOT + '/Materials')

def material(name, color, roughness=.85, metallic=0, paper=False):
    path = ROOT + '/Materials/' + name
    m = unreal.load_asset(path)
    if m:
        return m
    m = assets.create_asset(name, ROOT + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    def expr(cls, x=0, y=0):
        return lib.create_material_expression(m, cls, x, y)
    tint = expr(unreal.MaterialExpressionVectorParameter, -500, 0)
    tint.set_editor_property('parameter_name', 'Tint')
    tint.set_editor_property('default_value', unreal.LinearColor(*color, 1))
    source = tint
    if paper:
        world = expr(unreal.MaterialExpressionWorldPosition, -700, 200)
        normal = expr(unreal.MaterialExpressionVertexNormalWS, -700, 400)
        custom = expr(unreal.MaterialExpressionCustom, -200, 0)
        custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        inputs=[]
        for name in ['P','N','Tint']:
            entry=unreal.CustomInput()
            entry.set_editor_property('input_name',name)
            inputs.append(entry)
        custom.set_editor_property('inputs', inputs)
        custom.set_editor_property('code', '''
float stripe = smoothstep(.32,.44,frac(P.z/11));
float grain = frac(sin(dot(floor(P.xy*1.8),float2(12.9898,78.233)))*43758.5453);
float3 cardboard = lerp(float3(.40,.23,.12),float3(.68,.46,.27),stripe);
float top = saturate((abs(N.z)-.8)*5);
float2 cell=frac(P.xy/200);
float seam = step(.012,min(min(cell.x,1-cell.x),min(cell.y,1-cell.y)));
return lerp(cardboard,Tint*(.94+.06*grain)*lerp(.75,1,seam),top);
''')
        lib.connect_material_expressions(world, '', custom, 'P')
        lib.connect_material_expressions(normal, '', custom, 'N')
        lib.connect_material_expressions(tint, '', custom, 'Tint')
        source = custom
    lib.connect_material_property(source, '', unreal.MaterialProperty.MP_BASE_COLOR)
    for value, prop, y in [(roughness, unreal.MaterialProperty.MP_ROUGHNESS, 200), (metallic, unreal.MaterialProperty.MP_METALLIC, 300)]:
        c = expr(unreal.MaterialExpressionConstant, -200, y)
        c.set_editor_property('r', value)
        lib.connect_material_property(c, '', prop)
    lib.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

paper = material('M_Paper', (.66,.79,.66), paper=True)
metal = material('M_Metal', (.26,.34,.39), .27, .85)
exit_mat = material('M_Exit', (.13,.65,.43), .5)
desk_mat = material('M_Desk', (.29,.17,.12), .75)
back_mat = material('M_Background', (.68,.70,.64), .9)
coral = material('M_Coral', (.7,.27,.23), .85)
cream = material('M_Cream', (.87,.79,.6), .9)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
path = ROOT + '/LV_SNIP_Prototype'
if unreal.EditorAssetLibrary.does_asset_exist(path):
    levels.load_level(path)
else:
    levels.new_level(path)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing = {a.get_actor_label():a for a in actors.get_all_level_actors()}
def spawn(cls, label, loc, rot=unreal.Rotator()):
    a = existing.get(label) or actors.spawn_actor_from_class(cls, unreal.Vector(*loc), rot)
    a.set_actor_label(label)
    return a
def cube(label, loc, scale, mat, rot=unreal.Rotator()):
    a=spawn(unreal.StaticMeshActor, label, loc, rot)
    c=a.static_mesh_component
    c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    c.set_material(0,mat)
    c.set_collision_profile_name('NoCollision')
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    a.set_actor_enable_collision(False)
    a.tags=['SNIPDecoration']
    a.set_actor_scale3d(unreal.Vector(*scale))
    return a

grid=spawn(unreal.load_class(None,'/Script/DirectorsCutJAM.SNIPPuzzle'),'SNIP / Active Puzzle',(0,0,0))
grid.set_editor_property('stage',0)
grid.build_puzzle()
spawn(unreal.PlayerStart,'SNIP / Entrance',(300,500,290),unreal.Rotator(0,0,0))
cube('SNIP / Walnut Worktable',(1000,650,-650),(65,55,2),desk_mat)
cube('SNIP / Cutting Mat',(1000,650,-538),(34,28,.2),back_mat)
cube('SNIP / Backdrop',(1000,650,-850),(800,800,1),cream)
cube('SNIP / Pencil',(-1100,0,-525),(.35,16,.35),coral,unreal.Rotator(0,16,0))
cube('SNIP / Paper Stack',(2900,1300,-493),(8,9,.8),cream,unreal.Rotator(0,-13,0))
cube('SNIP / Coral Scrap',(2900,-500,-532),(5,4,.04),coral,unreal.Rotator(0,24,0))

sun=spawn(unreal.DirectionalLight,'SNIP / Softbox',(1000,-500,3200),unreal.Rotator(-55,-35,0))
sun.light_component.set_editor_property('intensity',3.5)
sun.light_component.set_editor_property('light_color',unreal.Color(255,238,215))
sun.light_component.set_editor_property('light_source_angle',6)
sky=spawn(unreal.SkyLight,'SNIP / Fill',(0,0,2000))
sky.light_component.set_editor_property('intensity',1.2)
sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sky.light_component.set_editor_property('cubemap',unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
pp=spawn(unreal.PostProcessVolume,'SNIP / Exposure',(0,0,0))
pp.set_editor_property('unbound',True)
s=pp.get_editor_property('settings')
s.set_editor_property('override_auto_exposure_min_brightness',True)
s.set_editor_property('override_auto_exposure_max_brightness',True)
s.set_editor_property('auto_exposure_min_brightness',.5)
s.set_editor_property('auto_exposure_max_brightness',.5)
pp.set_editor_property('settings',s)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/DirectorsCutJAM.SNIPGameMode'))
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(unreal.Vector(-1900,-2200,2800),unreal.Rotator(-35,42,0))
levels.save_current_level()
unreal.log('SNIP_BUILD_COMPLETE: saved integrated map, puzzle actor, PlayerStart, native GameMode, materials and lighting.')
