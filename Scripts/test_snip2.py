"""Gameplay 2.0 live PIE regression. Real mouse/keyboard and native collision."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
scope={}
exec(compile((root/'Scripts/test_snip_mouse.py').read_text().split('snip_mouse_probe=MouseProbe()')[0],'mouse_helpers','exec'),scope)
MouseProbe=scope['MouseProbe']
course={}
exec(compile((root/'Scripts/test_snip_course.py').read_text().split('snip_course_probe=CourseProbe()')[0],'course_helpers','exec'),course)
class Snip2Probe(MouseProbe):
    report_name='SNIP_Gameplay2'
    walk=course['CourseProbe'].walk
    def reset(self,stage=0,view=2):
        self.g.set_editor_property('stage',stage);self.g.restart();self.p.set_view(view)
    def drag(self,start,end):
        self.mouse(unreal.Vector(*start));yield .15
        self.button(True);yield .15
        self.mouse(unreal.Vector(*end));yield .2
    def run(self):
        unreal.WidgetLibrary.set_focus_to_game_viewport()
        self.reset();yield 1
        self.check('Native pawn and HUD integrated',self.p.get_class().get_name()=='SNIPCharacter' and self.pc.get_hud().get_class().get_name()=='SNIPHUD')
        for mode,axis,start,end in [(1,0,(900,500,300),(1700,500,300)),(2,0,(1700,500,300),(900,500,300)),(3,2,(1000,500,500),(1000,500,700)),(2,1,(400,700,300),(400,1300,300))]:
            self.reset(1 if axis==2 else 2 if axis==1 else 0,mode);yield .9
            before=self.g.get_editor_property('dimensions');dims=(before.x,before.y,before.z)
            yield from self.drag(start,end)
            lo=self.p.get_editor_property('low');hi=self.p.get_editor_property('high')
            self.check('Inclusive endpoints view %s axis %s'%(mode,axis),self.p.get_editor_property('axis')==axis and (lo,hi)==((2,4) if axis==2 else (3,7) if axis==1 else (4,9)),interval=[lo,hi])
            self.button(False);yield .8
            after=self.g.get_editor_property('dimensions')
            self.check('Release immediately cuts view %s axis %s'%(mode,axis),(after.x,after.y,after.z)[axis]==dims[axis]-(hi-lo))
        self.reset();yield .9
        yield from self.drag((900,500,100),(1500,500,100))
        self.mouse(unreal.Vector(920,850,100));yield .3
        self.check('Direction switches during drag',self.p.get_editor_property('axis')==1 and self.p.get_editor_property('low')==2)
        self.button(True,True);self.button(False,True);self.button(False);yield .6
        self.check('RMB suppresses release',self.g.get_editor_property('dimensions').x==12 and self.g.get_editor_property('dimensions').y==5)
        yield from self.drag((900,500,100),(940,500,100))
        self.check('Deliberate one-cell interval',self.p.get_editor_property('high')-self.p.get_editor_property('low')==1)
        self.button(False);yield .7
        self.check('One-cell release cuts',self.g.get_editor_property('dimensions').x==11)
        self.reset();yield .8
        before=self.state()
        self.check('Cut through performer accepted',self.g.apply_cut(0,1,2));yield .7
        self.check('Performer survives and is mapped safely',self.p.get_actor_location().x>=200 and self.p.get_actor_location().x<400)
        self.p.set_actor_location(unreal.Vector(500,500,290),False,False)
        pos=self.p.get_actor_location();self.g.undo();yield .4
        self.check('Undo preserves current valid position',self.p.get_actor_location().distance(pos)<1 and self.state()==before)
        self.g.redo();yield .6
        self.check('Redo preserves current valid position',self.p.get_actor_location().distance(pos)<1)
        self.g.undo();yield .4
        self.g.apply_cut(0,4,9);yield .7
        saved=self.state();self.g.redo();yield .4
        self.check('New cut clears redo branch',self.state()==saved)
        self.reset(1,1);yield .8
        saved=self.state()
        self.check('Metal rejected',not self.g.apply_cut(2,4,5) and self.state()==saved)
        self.check('Metal translates above paper cut',self.g.apply_cut(2,2,4));yield .7
        self.check('Metal survives at exact lowered grid',all(pos[2]==2 for pos,alive,metal in self.state() if alive and metal))
        self.reset(3,0);yield .7
        self.p.jump();yield .15
        movement=self.p.get_editor_property('character_movement');v=movement.velocity
        self.p.set_view(1);pos=self.p.get_actor_location();yield .4
        self.check('Airborne edit freezes position',self.p.get_actor_location().distance(pos)<1)
        self.p.set_view(0)
        self.check('Airborne edit restores velocity',movement.velocity.distance(v)<1)
        yield 1.5
        self.reset(3,1);yield .8
        pos=self.p.get_actor_location()
        self.check('Removing player support accepted',self.g.apply_cut(2,5,6));yield .7
        self.p.set_view(0);yield .5
        self.check('Deleted support resumes gravity',self.p.get_actor_location().z<pos.z-150)
        for fps in [30,60,120]:
            self.reset();yield .7
            unreal.SystemLibrary.execute_console_command(self.world,'t.MaxFPS '+str(fps))
            self.g.apply_cut(0,4,9);yield .8
            dt=unreal.GameplayStatics.get_world_delta_seconds(self.world)
            self.check('Measured timestep near target %s FPS'%fps,abs(dt-1/fps)<.008,delta_seconds=dt)
            cells=self.g.get_editor_property('cells')
            self.check('Exact final geometry at %s FPS'%fps,all(c.get_editor_property('mesh').get_world_location().distance(unreal.Vector(c.get_editor_property('grid').x*200+100,c.get_editor_property('grid').y*200+100,c.get_editor_property('grid').z*200+100))<.1 for c in cells if c.get_editor_property('alive')))
        unreal.SystemLibrary.execute_console_command(self.world,'t.MaxFPS 60')
        # Existing three courses still use genuine CharacterMovement traversal.
        yield from course['CourseProbe'].run(self)
        self.reset(3,0);yield .8
        yield from self.walk(1500,500)
        self.p.set_view(1);yield .7
        self.check('Trapdoor solution cut',self.g.apply_cut(2,5,6));yield .7
        self.p.set_view(0);yield 1.6
        self.check('Trapdoor exit reached by falling',self.g.get_editor_property('complete'),position=str(self.p.get_actor_location()))
        yield 2.5
        self.check('Progresses to fifth puzzle',self.g.get_editor_property('stage')==4)
        self.reset(4,1);yield .8
        old=self.p.get_actor_location()
        self.check('Riding island lower cut',self.g.apply_cut(2,2,4));yield .7
        self.check('Player carried with surviving island',abs(self.p.get_actor_location().z-(old.z-400))<2)
        self.p.set_view(2);yield .7
        self.check('Riding island depth cut',self.g.apply_cut(1,3,9));yield .7
        self.p.set_view(0);yield .7
        yield from self.walk(1900,900)
        self.check('Fifth puzzle completed by walking and falling',self.g.get_editor_property('complete'),position=str(self.p.get_actor_location()))
        self.reset(0,0)
snip2_probe=Snip2Probe()
