import unreal,json,time
from pathlib import Path
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
p=unreal.GameplayStatics.get_player_character(w,0);pc=p.get_controller();m=p.get_editor_property('character_movement');g=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.load_class(None,'/Script/DirectorsCutJAM.SNIPPuzzle'))[0]
rows=[];start=time.monotonic()
def sample(dt):
    a=p.get_editor_property('mesh').get_anim_instance()
    row=dict(t=time.monotonic()-start,pos=str(p.get_actor_location()),vel=str(p.get_velocity()),accel=str(m.get_current_acceleration()),mode=str(m.get_editor_property('movement_mode')),view=p.get_editor_property('view'),axis=p.get_editor_property('axis'),lo=p.get_editor_property('low'),hi=p.get_editor_property('high'),locked=p.get_editor_property('locked'),root_motion=str(a.get_editor_property('root_motion_mode')) if a else None,anim=a.get_class().get_name() if a else None,root=str(p.get_editor_property('mesh').get_socket_location('root')),pelvis=str(p.get_editor_property('mesh').get_socket_location('pelvis')),mesh=str(p.get_editor_property('mesh').get_world_location()),mouse=str(pc.get_mouse_position()),notice=str(g.get_editor_property('notice')))
    rows.append(row)
    if time.monotonic()-start>2:
        unreal.unregister_slate_post_tick_callback(handle)
        Path(unreal.Paths.project_saved_dir()+'Logs/SNIP_Diagnosis.json').write_text(json.dumps(rows,indent=2))
        unreal.log('SNIP_DIAG '+json.dumps(row))
handle=unreal.register_slate_post_tick_callback(sample)


