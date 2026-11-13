"""Capture the actual PIE views, then return to puzzle one."""
import unreal,time
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
p=unreal.GameplayStatics.get_player_character(world,0)
g=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/DirectorsCutJAM.SNIPPuzzle'))[0]
g.set_editor_property('stage',0);g.restart()
unreal.SystemLibrary.execute_console_command(world,'viewmode lit')
start=time.monotonic();phase=0
def capture(dt):
    global start,phase
    if time.monotonic()-start<2:return
    if phase==0:
        unreal.SystemLibrary.execute_console_command(world,'Shot filename=SNIP_Opening.png')
    elif phase==1:
        g.set_editor_property('stage',1);g.restart();p.set_view(1)
    elif phase==2:
        unreal.SystemLibrary.execute_console_command(world,'Shot filename=SNIP_Front.png')
    else:
        g.set_editor_property('stage',0);g.restart()
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.log('SNIP_FINAL_CAPTURE_COMPLETE')
    start=time.monotonic();phase+=1
handle=unreal.register_slate_post_tick_callback(capture)
