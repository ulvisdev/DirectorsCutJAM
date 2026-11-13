import unreal,ctypes
from pathlib import Path
root=Path(unreal.Paths.project_dir())
scope={}
exec(compile((root/'Scripts/test_snip_mouse.py').read_text().split('snip_mouse_probe=MouseProbe()')[0],'mouse','exec'),scope)
class CinemaUIProbe(scope['MouseProbe']):
    report_name='SNIP_CinematicUI'
    def click_screen(self,x,y):
        unreal.WidgetLibrary.set_focus_to_game_viewport()
        self.pc.set_mouse_location(int(x),int(y))
        class POINT(ctypes.Structure):_fields_=[('x',ctypes.c_long),('y',ctypes.c_long)]
        pt=POINT();ctypes.windll.user32.GetCursorPos(ctypes.byref(pt));ctypes.windll.user32.ScreenToClient(ctypes.c_void_p(self.hwnd),ctypes.byref(pt))
        ctypes.windll.user32.PostMessageW(ctypes.c_void_p(self.hwnd),0x200,0,(pt.y<<16)|(pt.x&0xffff))
        yield .2
        self.check('Mouse target delivered',True,target=[x,y],actual=str(self.pc.get_mouse_position()),viewport=str(self.pc.get_viewport_size()))
        self.button(True);yield .2;self.button(False);yield .3
    def run(self):
        f=self.p.get_editor_property('presentation')
        self.check('Title available after full film',f.get_editor_property('title_menu'))
        w,h=self.pc.get_viewport_size();step=min(56,h*.065)
        yield from self.click_screen(w*.5,h*.48+step+8)
        self.check('Mouse opens Controls',f.get_editor_property('menu_page')==1)
        unreal.SystemLibrary.execute_console_command(self.world,'Shot filename=SNIP_Cinematic_Controls.png')
        yield from self.click_screen(w*.5,h*.86+8)
        self.check('Mouse Back from Controls',f.get_editor_property('menu_page')==0)
        if f.get_editor_property('menu_page')!=0:return
        yield from self.click_screen(w*.5,h*.48+step*2+8)
        self.check('Mouse opens Settings',f.get_editor_property('menu_page')==2)
        yield from self.click_screen(w*.5,h*.48+step*2+8)
        self.check('Skip tutorial available in settings',f.get_editor_property('tutorial_step')==5)
        yield from self.click_screen(w*.5,h*.48+step*3+8)
        yield from self.click_screen(w*.5,h*.48+8)
        self.check('Mouse Play starts new film',not f.get_editor_property('menu_open') and self.g.get_editor_property('stage')==0)
        if f.get_editor_property('menu_open'):return
        yield .5
        unreal.SystemLibrary.execute_console_command(self.world,'Shot filename=SNIP_Cinematic_CleanPlay.png')
        self.p.set_view(2);yield .4
        self.mouse(unreal.Vector(900,500,100));yield .15;self.button(True);yield .15
        self.mouse(unreal.Vector(1700,500,100));yield .2
        saved=self.state()
        unreal.SystemLibrary.execute_console_command(self.world,'Shot filename=SNIP_Cinematic_Ghosts.png')
        yield .4
        self.check('Ghost preview leaves logical state unchanged',self.state()==saved)
        self.button(True,True);self.button(False,True);self.button(False);yield .2
        self.check('Cancel still suppresses release',self.state()==saved)
        # Merely changing editing cameras below the boundary must not trigger a failed take.
        deaths=f.get_editor_property('deaths')
        self.p.set_actor_location(unreal.Vector(300,500,-600),False,False)
        self.p.set_view(3);yield .4
        self.check('Editing cannot accidentally trigger death',f.get_editor_property('deaths')==deaths and self.p.get_actor_location().z==-600)
        self.p.set_view(0);yield .8
        self.check('Returning to play below boundary resets once',f.get_editor_property('deaths')==deaths+1 and self.p.get_actor_location().z>280)
        self.p.set_actor_location(unreal.Vector(300,500,-600),False,False);yield .8
        self.check('Repeated deaths offer restart hint',f.get_editor_property('deaths')>=2 and 'RESTART' in f.get_editor_property('prompt'))
        self.p.set_view(2);yield .4
        self.g.apply_cut(0,4,9);yield .65
        self.press(0x52);yield .6
        self.check('R key restores active puzzle and gameplay view',self.g.get_editor_property('dimensions').x==12 and self.p.get_editor_property('view')==0)
        yield 3.5
        self.check('Contextual hints expire',f.get_editor_property('prompt')=='')
        f.toggle_menu()
cinema_ui_probe=CinemaUIProbe()
