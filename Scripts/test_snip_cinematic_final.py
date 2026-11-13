import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
main={};ui={}
exec(compile((root/'Scripts/test_snip_cinematic.py').read_text().split('cinematic_probe=CinematicProbe()')[0],'cinematic','exec'),main)
exec(compile((root/'Scripts/test_snip_cinematic_ui.py').read_text().split('cinema_ui_probe=CinemaUIProbe()')[0],'cinematic_ui','exec'),ui)
class FinalProbe(main['CinematicProbe']):
    report_name='SNIP_CinematicFinal'
    click_screen=ui['CinemaUIProbe'].click_screen
    def run(self):
        yield from super().run()
        yield from ui['CinemaUIProbe'].run(self)
        self.f.toggle_menu();self.g.build_puzzle();self.p.set_view(2);yield .4
        self.check('Tutorial prop module can be removed',self.g.apply_cut(0,2,3));yield .65
        self.p.set_actor_location(unreal.Vector(500,500,290),False,False)
        self.g.undo();yield .35
        pos=self.p.get_actor_location()
        self.check('Undo resolves restored tutorial obstacle safely',pos.z>=348 and pos.distance(unreal.Vector(500,500,290))<100,position=str(pos))
        self.p.set_view(0);yield .5
        self.check('Restored obstacle collision supports performer',self.p.get_actor_location().distance(pos)<4)
        self.g.restart();yield .6
        self.f.toggle_menu()
cinematic_final=FinalProbe()
