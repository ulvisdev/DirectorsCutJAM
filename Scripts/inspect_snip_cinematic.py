import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir())
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
grid=next(a for a in actors if a.get_class().get_name()=='SNIPPuzzle')
data={
    'project':unreal.Paths.get_base_filename(unreal.Paths.get_project_file_path()),
    'world':world.get_path_name(),
    'game_mode':world.get_world_settings().get_editor_property('default_game_mode').get_path_name(),
    'puzzle_actor':grid.get_path_name(),
    'stage':grid.get_editor_property('stage'),
    'paper_material':grid.get_editor_property('paper_material').get_path_name(),
    'metal_material':grid.get_editor_property('metal_material').get_path_name(),
    'material_assets':list(unreal.EditorAssetLibrary.list_assets('/Game/SNIP/Cinematic/Materials',recursive=True,include_folder=False)),
    'decoration_collision_disabled':all(not a.get_actor_enable_collision() for a in actors if a.actor_has_tag('SNIPDecoration')),
    'player_start_present':any(a.get_class().get_name()=='PlayerStart' for a in actors),
}
(root/'Docs/Validation/SNIP_CinematicIntegration.json').write_text(json.dumps(data,indent=2))
unreal.log('SNIP_CINEMATIC_INSPECTION '+json.dumps(data))
