import unreal
from pathlib import Path
scope={}
exec(compile((Path(unreal.Paths.project_dir())/'Scripts/test_snip_mouse.py').read_text().split('snip_mouse_probe=MouseProbe()')[0],'helpers','exec'),scope)
class ObstacleProbe(scope['MouseProbe']):
    report_name='SNIP_CinematicObstacle'
    def run(self):
        f=self.p.get_editor_property('presentation')
        if f.get_editor_property('menu_open'):f.toggle_menu()
        f.skip_tutorial()
        self.g.set_editor_property('stage',0);self.g.build_puzzle();self.p.set_view(2);yield .5
        self.check('Tutorial prop module can be removed',self.g.apply_cut(0,2,3));yield .7
        self.p.set_actor_location(unreal.Vector(500,500,290),False,False)
        self.g.undo();yield .4
        pos=self.p.get_actor_location()
        self.check('Restored obstacle resolves overlap nearby',pos.z>=348 and pos.distance(unreal.Vector(500,500,290))<100,position=str(pos))
        self.p.set_view(0);yield .6
        self.check('Restored obstacle supports performer',self.p.get_actor_location().distance(pos)<4)
obstacle_probe=ObstacleProbe()
