import unreal
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
count=0
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.StaticMeshActor) and a.get_actor_label().startswith('SNIP /'):
        a.static_mesh_component.set_collision_profile_name('NoCollision')
        a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        a.set_actor_enable_collision(False)
        tags=list(a.tags)
        if 'SNIPDecoration' not in [str(t) for t in tags]:tags.append('SNIPDecoration')
        a.tags=tags
        count+=1
assert count>=7
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('SNIP_COLLISION_REPAIRED '+str(count)+' saved decorative actors')
