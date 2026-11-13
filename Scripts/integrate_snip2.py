import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
exec(compile((root/'Scripts/create_snip_audio.py').read_text(),str(root/'Scripts/create_snip_audio.py'),'exec'),{'__file__':str(root/'Scripts/create_snip_audio.py')})
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in sub.get_all_level_actors():
    if actor.get_class().get_name()=='SNIPPuzzle':
        actor.set_editor_property('compression_seconds',.20)
        actor.set_editor_property('stage',0)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('SNIP_GAMEPLAY2_INTEGRATED')
