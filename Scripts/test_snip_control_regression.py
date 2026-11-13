import unreal
exec(compile(open(unreal.Paths.project_dir()+'Scripts/test_snip_pie.py').read().split('snip_probe=SNIPProbe()')[0],'probe','exec'))
class ControlProbe(SNIPProbe):
    report_name='SNIP_ControlRegression'
    def run(self):
        self.g.set_editor_property('stage',0);self.g.restart();unreal.WidgetLibrary.set_focus_to_game_viewport()
        yield .7
        decor=[a for a in unreal.GameplayStatics.get_all_actors_of_class(self.world,unreal.Actor) if a.actor_has_tag('SNIPDecoration')]
        self.check('Saved decoration cannot block the pawn',len(decor)>=7 and all(not a.get_actor_enable_collision() for a in decor),count=len(decor))
        before=self.p.get_actor_location()
        self.key(0x57);yield .6;self.key(0x57,False);yield .5
        stopped=self.p.get_actor_location()
        self.check('WASD actually moves character',stopped.distance(before)>80,distance=stopped.distance(before))
        root=self.p.get_editor_property('mesh').get_socket_location('root')
        self.check('Walk animation remains inside capsule',abs(root.x-stopped.x)<40 and abs(root.y-stopped.y)<40,offset=str(root-stopped))
        yield .7
        self.check('Released movement stops',self.p.get_actor_location().distance(stopped)<3,distance=self.p.get_actor_location().distance(stopped))
        self.g.restart();yield .7
        start=self.p.get_actor_location();self.key(0x20);peak=start.z
        for i in range(10):
            yield .06
            pos=self.p.get_actor_location();root=self.p.get_editor_property('mesh').get_socket_location('root')
            peak=max(peak,pos.z)
            self.check('Standing jump sample %d'%i,abs(pos.x-start.x)<3 and abs(pos.y-start.y)<3 and abs(root.x-pos.x)<40 and abs(root.y-pos.y)<40,pos=str(pos),offset=str(root-pos))
        self.key(0x20,False);yield 1
        self.check('Standing jump rises and lands vertically',peak>start.z+70 and abs(self.p.get_actor_location().z-start.z)<4,peak=peak-start.z)
        self.g.restart();self.p.set_view(2);yield .8
        self.check('Empty bridge slab valid',self.g.validate_cut(0,4,9)=='')
snip_control_probe=ControlProbe()
