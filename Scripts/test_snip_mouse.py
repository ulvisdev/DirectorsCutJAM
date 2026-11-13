"""Exercise real viewport mouse selection, release, confirmation and cancellation."""
import unreal
exec(compile(open(unreal.Paths.project_dir()+'Scripts/test_snip_pie.py').read().split('snip_probe=SNIPProbe()')[0], 'test_snip_pie.py', 'exec'))
class MouseProbe(SNIPProbe):
    report_name='SNIP_Mouse'
    def mouse(self, world):
        screen=self.pc.project_world_location_to_screen(world)
        self.pc.set_mouse_location(int(screen.x),int(screen.y))
        class POINT(ctypes.Structure):_fields_=[('x',ctypes.c_long),('y',ctypes.c_long)]
        pt=POINT();ctypes.windll.user32.GetCursorPos(ctypes.byref(pt));ctypes.windll.user32.ScreenToClient(ctypes.c_void_p(self.hwnd),ctypes.byref(pt))
        ctypes.windll.user32.PostMessageW(ctypes.c_void_p(self.hwnd),0x200,1 if self.p.get_editor_property('selecting') else 0,(pt.y<<16)|(pt.x&0xffff))
    def button(self, down, right=False):
        class POINT(ctypes.Structure):_fields_=[('x',ctypes.c_long),('y',ctypes.c_long)]
        pt=POINT();ctypes.windll.user32.GetCursorPos(ctypes.byref(pt));ctypes.windll.user32.ScreenToClient(ctypes.c_void_p(self.hwnd),ctypes.byref(pt))
        message=(0x204 if down else 0x205) if right else (0x201 if down else 0x202)
        ctypes.windll.user32.PostMessageW(ctypes.c_void_p(self.hwnd),message,(2 if right else 1) if down else 0,(pt.y<<16)|(pt.x&0xffff))
    def finish(self):
        super().finish()
        Path(unreal.Paths.project_saved_dir()+'Logs/SNIP_Mouse.json').write_text(json.dumps(self.results,indent=2))
    def run(self):
        self.g.set_editor_property('stage',1);self.g.restart();self.p.set_view(1);yield 1
        self.mouse(unreal.Vector(1000,500,800));yield .3
        self.button(True);yield .3
        self.mouse(unreal.Vector(1000,500,1000));yield .4
        self.button(False);yield .4
        self.check('Mouse band identifies metal obstruction',self.p.get_editor_property('locked') and 'Metal' in self.g.validate_cut(self.p.get_editor_property('axis'),self.p.get_editor_property('low'),self.p.get_editor_property('high')))
        unreal.SystemLibrary.execute_console_command(self.world,'Shot filename=SNIP_InvalidMetal.png')
        self.button(True);self.button(False);yield .8
        self.check('Mouse confirmation cannot cut metal',self.g.get_editor_property('dimensions').z==8 and 'Metal' in str(self.g.get_editor_property('notice')))
        self.g.set_editor_property('stage',0);self.g.restart()
        unreal.WidgetLibrary.set_focus_to_game_viewport()
        self.press(0x33)
        yield 1.2
        self.mouse(unreal.Vector(800,500,100))
        yield .3
        self.button(True)
        yield .3
        self.mouse(unreal.Vector(1800,500,100))
        yield .5
        self.check('Mouse drag selects X interval',self.p.get_editor_property('selecting') and self.p.get_editor_property('axis')==0 and self.p.get_editor_property('low')==4 and self.p.get_editor_property('high')==9,low=self.p.get_editor_property('low'),high=self.p.get_editor_property('high'))
        self.button(False)
        yield .4
        self.check('Mouse release locks selection',self.p.get_editor_property('locked'))
        self.button(True);self.button(False)
        yield .8
        self.check('Second click executes compression',self.g.get_editor_property('dimensions').x==7,notice=str(self.g.get_editor_property('notice')))
        self.press(0x5a)
        yield .8
        self.mouse(unreal.Vector(800,500,100));yield .3
        self.button(True);yield .3
        self.mouse(unreal.Vector(1800,500,100));yield .3
        self.button(False);yield .3
        self.button(True,True);self.button(False,True);yield .3
        self.check('RMB cancels locked band',not self.p.get_editor_property('locked') and self.g.get_editor_property('dimensions').x==12)
        for stage,mode,axis,lo,hi,start,end in [
            (0,1,0,4,9,(800,500,300),(1800,500,300)),
            (1,1,2,2,4,(1000,500,400),(1000,500,800)),
            (2,2,1,3,7,(400,600,300),(400,1400,300)),
            (2,3,1,3,7,(400,600,300),(400,1400,300)),
            (1,3,2,2,4,(1000,500,400),(1000,500,800)),
        ]:
            self.g.set_editor_property('stage',stage);self.g.restart();self.p.set_view(mode)
            yield 1
            self.mouse(unreal.Vector(*start));yield .3
            self.button(True);yield .3
            self.mouse(unreal.Vector(*end));yield .4
            self.button(False);yield .4
            self.check('Drag mapping view %d axis %d'%(mode,axis),self.p.get_editor_property('locked') and self.p.get_editor_property('axis')==axis and self.p.get_editor_property('low')==lo and self.p.get_editor_property('high')==hi,low=self.p.get_editor_property('low'),high=self.p.get_editor_property('high'))
            dim=self.g.get_editor_property('dimensions');before=(dim.x,dim.y,dim.z)
            self.button(True);self.button(False);yield .8
            after=self.g.get_editor_property('dimensions')
            self.check('Click applies view %d axis %d'%(mode,axis),[after.x,after.y,after.z][axis]==before[axis]-(hi-lo))
        self.g.set_editor_property('stage',0);self.g.restart()
        self.p.set_view(0)

snip_mouse_probe=MouseProbe()
