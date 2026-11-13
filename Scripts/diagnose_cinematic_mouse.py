import unreal
from pathlib import Path
scope={}
exec(compile((Path(unreal.Paths.project_dir())/'Scripts/test_snip_cinematic_ui.py').read_text().split('cinema_ui_probe=CinemaUIProbe()')[0],'ui','exec'),scope)
class MouseDiagnostic(scope['CinemaUIProbe']):
    report_name='SNIP_CinematicMouseDiagnostic'
    def run(self):
        self.button(False);yield .2
        f=self.p.get_editor_property('presentation')
        self.check('initial',True,menu=f.get_editor_property('menu_open'),page=f.get_editor_property('menu_page'),size=str(self.pc.get_viewport_size()))
        w,h=self.pc.get_viewport_size()
        for label,y in [('controls',h*.48+min(56,h*.065)+8),('back',h*.86+8),('back repeat',h*.86+8)]:
            self.pc.set_mouse_location(int(w*.5),int(y));yield .3
            self.check(label+' before',True,target_y=y,mouse=str(self.pc.get_mouse_position()),page=f.get_editor_property('menu_page'))
            self.button(True);yield .3
            self.check(label+' held',True,mouse=str(self.pc.get_mouse_position()),page=f.get_editor_property('menu_page'))
            self.button(False);yield .3
mouse_diagnostic=MouseDiagnostic()
