"""Read-only saved-level integration inventory. Run outside PIE."""
import unreal,json
from pathlib import Path
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
mode=world.get_world_settings().get_editor_property('default_game_mode')
defaults=unreal.get_default_object(mode)
data=dict(level=world.get_path_name(),game_mode=mode.get_path_name(),pawn=defaults.get_editor_property('default_pawn_class').get_path_name(),hud=defaults.get_editor_property('hud_class').get_path_name(),actors=[dict(label=a.get_actor_label(),type=a.get_class().get_name()) for a in actors])
Path(unreal.Paths.project_saved_dir()+'Logs/SNIP_EditorIntegration.json').write_text(json.dumps(data,indent=2))
assert sum(a.get_class().get_name()=='SNIPPuzzle' for a in actors)==1
assert sum(a.get_class().get_name()=='PlayerStart' for a in actors)==1
assert mode.get_name()=='SNIPGameMode'
assert defaults.get_editor_property('default_pawn_class').get_name()=='SNIPCharacter'
assert defaults.get_editor_property('hud_class').get_name()=='SNIPHUD'
unreal.log('SNIP_EDITOR_INTEGRATION_VERIFIED '+json.dumps(data))
