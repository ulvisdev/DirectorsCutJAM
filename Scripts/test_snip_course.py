"""Traverse solved puzzles with CharacterMovement collision and jumps, without teleporting."""
import unreal
exec(compile(open(unreal.Paths.project_dir()+'Scripts/test_snip_pie.py').read().split('snip_probe=SNIPProbe()')[0], 'test_snip_pie.py', 'exec'))

class CourseProbe(SNIPProbe):
    report_name='SNIP_Course'
    def walk(self,x,y,jump=False):
        if jump:self.p.jump()
        start=time.monotonic()
        stage=self.g.get_editor_property('stage')
        while time.monotonic()-start<9:
            if self.g.get_editor_property('complete') or self.g.get_editor_property('stage')!=stage:break
            p=self.p.get_actor_location()
            d=unreal.Vector(x-p.x,y-p.y,0)
            if d.length()<30:break
            self.p.add_movement_input(d/max(.001,d.length()),1)
            yield .001
        self.p.stop_jumping()
        yield .4 if self.g.get_editor_property('complete') else .8
    def run(self):
        unreal.WidgetLibrary.set_focus_to_game_viewport()
        self.g.set_editor_property('stage',0);self.g.restart();self.p.set_view(2)
        yield .7
        self.check('Puzzle 1 solution accepted',self.g.apply_cut(0,4,9))
        yield .7
        self.p.set_view(0)
        yield from self.walk(1100,500)
        self.check('Puzzle 1 completed by walking',self.g.get_editor_property('complete'),position=str(self.p.get_actor_location()))
        yield 2.5
        self.check('Puzzle 2 automatic progression',self.g.get_editor_property('stage')==1)
        self.p.set_view(1);yield .7
        self.check('Puzzle 2 solution accepted',self.g.apply_cut(2,2,4));yield .7
        self.p.set_view(0)
        yield from self.walk(470,500)
        yield from self.walk(700,500,True)
        self.check('Jump onto paper step',abs(self.p.get_actor_location().z-490)<8,position=str(self.p.get_actor_location()))
        yield from self.walk(920,500,True)
        self.check('Jump onto lowered metal',abs(self.p.get_actor_location().z-690)<8,position=str(self.p.get_actor_location()))
        yield from self.walk(1300,500)
        self.check('Puzzle 2 exit reached through physics',self.g.get_editor_property('complete'),position=str(self.p.get_actor_location()))
        yield 2.5
        self.check('Puzzle 3 automatic progression',self.g.get_editor_property('stage')==2)
        self.p.set_view(2);yield .6
        self.check('Puzzle 3 Y cut accepted',self.g.apply_cut(1,3,7));yield .7
        self.check('Puzzle 3 X cut accepted',self.g.apply_cut(0,3,7));yield .7
        self.p.set_view(1);yield .6
        self.check('Puzzle 3 Z cut accepted',self.g.apply_cut(2,2,4));yield .7
        self.p.set_view(0)
        yield .6  # Let CharacterMovement reacquire its floor after editing.
        yield from self.walk(300,900)
        yield from self.walk(500,900,True)
        self.check('Final paper step reachable',abs(self.p.get_actor_location().z-490)<8,position=str(self.p.get_actor_location()))
        yield from self.walk(720,900,True)
        self.check('Final metal platform reachable',abs(self.p.get_actor_location().z-690)<8,position=str(self.p.get_actor_location()))
        yield from self.walk(900,900)
        self.check('Final PERFECT CUT state',self.g.get_editor_property('complete') and self.g.get_editor_property('stage')==2,position=str(self.p.get_actor_location()))
        yield 2
        self.g.set_editor_property('stage',0);self.g.restart()

snip_course_probe=CourseProbe()
