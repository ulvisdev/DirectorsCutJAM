import unreal,json
from pathlib import Path
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();p=unreal.GameplayStatics.get_player_character(w,0)
rows=[]
for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor):
    for c in a.get_components_by_class(unreal.PrimitiveComponent):
        if c.get_collision_enabled()!=unreal.CollisionEnabled.NO_COLLISION:
            rows.append(dict(actor=a.get_name(),component=c.get_name(),collision=str(c.get_collision_enabled()),loc=str(c.get_world_location()),scale=str(c.get_world_scale()),velocity=str(c.get_component_velocity())))
Path(unreal.Paths.project_saved_dir()+'Logs/SNIP_Collision.json').write_text(json.dumps(rows,indent=2))
unreal.log('SNIP_COLLISION_INSPECTED')
