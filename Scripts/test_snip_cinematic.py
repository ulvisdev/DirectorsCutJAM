"""Live PIE cinematic integration; no human-playthrough claim."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
scope={}
exec(compile((root/'Scripts/test_snip_mouse.py').read_text().split('snip_mouse_probe=MouseProbe()')[0],'mouse','exec'),scope)
course={}
exec(compile((root/'Scripts/test_snip_course.py').read_text().split('snip_course_probe=CourseProbe()')[0],'course','exec'),course)
class CinematicProbe(scope['MouseProbe']):
    report_name='SNIP_Cinematic'
    walk=course['CourseProbe'].walk
    def setup(self,stage=0,view=0):
        self.g.set_editor_property('stage',stage);self.g.build_puzzle()
        if view:self.p.set_view(view)
    def run(self):
        self.f=self.p.get_editor_property('presentation')
        self.m=self.p.get_editor_property('character_movement')
        unreal.get_default_object(unreal.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings')).set_editor_property('bThrottleCPUWhenNotForeground',False)
        unreal.WidgetLibrary.set_focus_to_game_viewport()
        self.check('Title menu integrated',self.f.get_editor_property('menu_open') and self.f.get_editor_property('title_menu'))
        start=self.p.get_actor_location();self.key(0x57);yield .25;self.key(0x57,False)
        self.check('Menu freezes performer',self.p.get_actor_location().distance(start)<.1)
        self.press(0x28);self.press(0x0d);yield .3
        self.check('Keyboard opens controls',self.f.get_editor_property('menu_page')==1)
        self.press(0x0d);yield .2
        self.press(0x28);self.press(0x28);self.press(0x0d);yield .2
        self.check('Settings navigation',self.f.get_editor_property('menu_page')==2)
        self.press(0x28);self.press(0x0d);yield .2
        self.check('Shake setting functional',self.g.get_editor_property('camera_shake_intensity')==0)
        self.press(0x0d);self.press(0x50);yield .2
        self.press(0x0d);yield .5
        self.check('PLAY closes menu',not self.f.get_editor_property('menu_open'))
        self.check('First movement prompt only',self.f.get_editor_property('prompt')=='WASD — MOVE')
        self.key(0x57);yield .4;self.key(0x57,False);yield .3
        self.check('Movement advances tutorial',self.f.get_editor_property('tutorial_step')==1)
        self.key(0x20);yield .12;self.key(0x20,False);yield .13
        self.check('Jump advances tutorial',self.f.get_editor_property('tutorial_step')==2)
        yield 1
        self.p.set_actor_location(unreal.Vector(700,500,290),False,False);yield .5
        self.check('Gap card freezes movement',self.f.get_editor_property('tutorial_blocking'))
        self.press(0x32);yield .4
        self.check('Required key works through blocking card',self.p.get_editor_property('view')==1 and self.f.get_editor_property('tutorial_step')==3 and not self.f.get_editor_property('tutorial_blocking'))
        self.mouse(unreal.Vector(900,500,300));yield .15;self.button(True);yield .15
        self.mouse(unreal.Vector(1700,500,300));yield .2
        self.button(False);yield .8
        self.check('Tutorial cut uses real release interaction',self.g.get_editor_property('dimensions').x==7 and self.f.get_editor_property('tutorial_step')==4)
        self.press(0x31);yield .5
        self.check('Return to film completes tutorial',self.f.get_editor_property('tutorial_step')==5)
        yield .4
        self.check('No permanent normal-play prompt',self.f.get_editor_property('prompt')=='')
        self.g.restart();yield .6
        self.check('Manual restart preserves learned tutorial',self.f.get_editor_property('tutorial_step')==5 and self.g.get_editor_property('dimensions').x==12)
        # Repeat each view without changing actual grounded position or walking mode.
        yield .3
        start=self.p.get_actor_location()
        for view in [1,2,3,0,3,1,2,0]:
            self.p.set_view(view);yield .4
            self.check('Grounded switch preserves position '+str(view),self.p.get_actor_location().distance(start)<.2,position=str(self.p.get_actor_location()))
            if view==0:self.check('Grounded resume remains Walking',self.m.get_editor_property('movement_mode')==unreal.MovementMode.MOVE_WALKING)
        # Airborne state is sampled, frozen across all edit cameras, then resumed.
        self.p.jump();yield .15
        pos=self.p.get_actor_location();v=self.m.velocity
        self.p.set_view(1);yield .35
        for view in [2,3,1]:
            self.p.set_view(view);yield .35
            self.check('Airborne edit remains stationary '+str(view),self.p.get_actor_location().distance(pos)<.1)
        self.p.set_view(0);yield .23
        self.check('Airborne resume keeps upward momentum',self.m.velocity.z>0 and abs(self.m.velocity.x-v.x)<1 and self.p.get_actor_location().z>=pos.z)
        yield 1
        self.p.stop_jumping()
        self.p.set_view(2);yield .4
        self.g.apply_cut(0,4,9);yield .7
        after=self.state();self.g.undo();yield .3;self.g.redo();yield .5
        self.check('History still restores exact edited grid',self.state()==after)
        self.p.set_view(0);yield .4
        self.p.set_actor_location(unreal.Vector(300,500,-600),False,False);yield .08
        self.check('Fall begins covered reset',self.f.get_editor_property('transition_active'))
        yield .6
        self.check('Death resets geometry history and entrance',self.g.get_editor_property('dimensions').x==12 and self.p.get_actor_location().distance(unreal.Vector(300,500,290))<2 and self.p.get_editor_property('view')==0)
        reset=self.state();self.g.undo();yield .3
        self.check('Death cleared undo history',self.state()==reset)
        self.check('Death does not replay tutorial',self.f.get_editor_property('tutorial_step')==5)
        # Pause is available under P in PIE (Escape is also an editor Stop binding).
        self.press(0x50);yield .2
        self.check('Pause opens',self.f.get_editor_property('menu_open'))
        self.press(0x28);self.press(0x0d);yield .2
        self.check('Pause shares controls panel',self.f.get_editor_property('menu_page')==1)
        self.press(0x50);self.press(0x50);yield .3
        self.check('Pause resumes movement',not self.f.get_editor_property('menu_open'))
        # Full first course includes the new small jump prop.
        self.p.set_view(2);yield .35
        self.g.apply_cut(0,4,9);yield .65
        self.p.set_view(0);yield .35
        yield from self.walk(700,500,True)
        yield from self.walk(1100,500)
        yield .8
        self.check('Exit seamlessly advances scene',self.g.get_editor_property('stage')==1 and not self.f.get_editor_property('transition_active'))
        self.p.set_view(1);yield .35
        self.check('Metal still rejects',not self.g.apply_cut(2,4,5));yield .4
        self.check('Brief metal feedback',"METAL" in self.f.get_editor_property('prompt'))
        self.g.apply_cut(2,2,4);yield .65
        self.p.set_view(0);yield .35
        yield from self.walk(470,500);yield from self.walk(700,500,True);yield from self.walk(920,500,True);yield from self.walk(1300,500)
        yield .8
        self.check('Second course still traversable',self.g.get_editor_property('stage')==2)
        self.p.set_view(2);yield .35
        self.g.apply_cut(1,3,7);yield .65
        self.g.apply_cut(0,3,7);yield .65
        self.p.set_view(1);yield .35
        self.g.apply_cut(2,2,4);yield .65
        self.p.set_view(0);yield .35
        yield from self.walk(300,900);yield from self.walk(500,900,True);yield from self.walk(720,900,True);yield from self.walk(900,900)
        yield .8
        self.check('Third course still traversable',self.g.get_editor_property('stage')==3)
        yield from self.walk(1500,500)
        self.p.set_view(1);yield .35
        self.g.apply_cut(2,5,6);yield .65
        self.p.set_view(0);yield 2
        self.check('Trapdoor progresses seamlessly',self.g.get_editor_property('stage')==4)
        self.p.set_view(1);yield .35
        self.g.apply_cut(2,2,4);yield .65
        self.p.set_view(2);yield .35
        self.g.apply_cut(1,3,9);yield .65
        self.p.set_view(0);yield .35
        yield from self.walk(1900,900);yield .8
        self.check('Final scene returns to title',self.f.get_editor_property('menu_open') and self.f.get_editor_property('title_menu'))
cinematic_probe=CinematicProbe()
