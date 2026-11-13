import unreal, json, time
from pathlib import Path
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
p=unreal.GameplayStatics.get_player_character(world,0)
g=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/DirectorsCutJAM.SNIPPuzzle'))[0]
pc=p.get_controller()
g.set_editor_property('stage',0);g.restart();p.set_view(2)
unreal.SystemLibrary.execute_console_command(world,'viewmode lit')
started=time.monotonic()
def capture(dt):
    if time.monotonic()-started<1.5:return
    unreal.unregister_slate_post_tick_callback(handle)
    cam=p.get_editor_property('view_camera')
    data=dict(camera=str(cam.get_world_location()),rotation=str(cam.get_world_rotation()),ortho=cam.get_editor_property('ortho_width'),player=str(p.get_actor_location()),viewport=str(pc.get_viewport_size()),points={str(x):str(pc.project_world_location_to_screen(unreal.Vector(x,500,100))) for x in [0,800,1200,1800,2400]})
    Path(unreal.Paths.project_saved_dir()+'Logs/SNIP_View.json').write_text(json.dumps(data,indent=2))
    unreal.SystemLibrary.execute_console_command(world,'Shot filename=SNIP_Top.png')
handle=unreal.register_slate_post_tick_callback(capture)
