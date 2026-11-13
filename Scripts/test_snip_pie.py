"""Live PIE integration checks, driven over editor ticks (never blocks Unreal).
Run `py <absolute path>` from the editor console after starting PIE.
Results: Saved/Logs/SNIP_PIE.json. This script modifies only the PIE world.
"""
import unreal, time, json, ctypes, traceback
from pathlib import Path

class SNIPProbe:
    report_name='SNIP_PIE'
    def __init__(self):
        self.world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        self.p=unreal.GameplayStatics.get_player_character(self.world,0)
        self.pc=self.p.get_controller()
        self.g=unreal.GameplayStatics.get_all_actors_of_class(self.world,unreal.load_class(None,'/Script/DirectorsCutJAM.SNIPPuzzle'))[0]
        self.results=[]
        self.keys=set()
        self.wait=0
        self.start=time.monotonic()
        ctypes.windll.user32.FindWindowW.restype=ctypes.c_void_p
        self.hwnd=ctypes.windll.user32.FindWindowW(None,'DirectorsCutJAM - Unreal Editor')
        self.steps=self.run()
        self.handle=unreal.register_slate_post_tick_callback(self.tick)
    def key(self,vk,down=True):
        if down:self.keys.add(vk)
        else:self.keys.discard(vk)
        scan=ctypes.windll.user32.MapVirtualKeyW(vk,0)
        ctypes.windll.user32.PostMessageW(ctypes.c_void_p(self.hwnd),0x100 if down else 0x101,vk,1|(scan<<16)|(0 if down else 0xC0000000))
    def press(self,vk):
        self.key(vk);self.key(vk,False)
    def check(self,name,ok,**data):
        result=dict(name=name,passed=bool(ok),**data)
        self.results.append(result)
        unreal.log('SNIP_TEST '+json.dumps(result))
    def state(self):
        cells=self.g.get_editor_property('cells')
        return [((c.get_editor_property('grid').x,c.get_editor_property('grid').y,c.get_editor_property('grid').z),c.get_editor_property('alive'),c.get_editor_property('metal')) for c in cells]
    def finish(self):
        for k in list(self.keys):self.key(k,False)
        unreal.unregister_slate_post_tick_callback(self.handle)
        Path(unreal.Paths.project_saved_dir()+'Logs/'+self.report_name+'.json').write_text(json.dumps(self.results,indent=2))
        unreal.log('SNIP_TEST_COMPLETE '+str(sum(r['passed'] for r in self.results))+'/'+str(len(self.results)))
    def tick(self,dt):
        if time.monotonic()<self.wait:return
        try:self.wait=time.monotonic()+next(self.steps)
        except StopIteration:self.finish()
        except Exception:
            self.check('probe exception',False,error=traceback.format_exc());self.finish()
    def run(self):
        self.g.set_editor_property('stage',0);self.g.restart()
        unreal.WidgetLibrary.set_focus_to_game_viewport()
        unreal.SystemLibrary.execute_console_command(self.world,'t.MaxFPS 60')
        self.check('Native pawn possessed',self.p.get_class().get_name()=='SNIPCharacter' and self.pc.get_controlled_pawn()==self.p)
        self.check('Native HUD assigned',self.pc.get_hud().get_class().get_name()=='SNIPHUD')
        self.check('Cutting disabled in normal view',not self.g.apply_cut(0,4,9))
        yield .8
        start=self.p.get_actor_location()
        self.key(0x57)
        yield .6
        self.key(0x57,False)
        end=self.p.get_actor_location()
        self.check('W movement through input',end.distance(start)>80,distance=end.distance(start))
        self.g.restart()
        yield .7
        base=self.p.get_actor_location().z
        self.key(0x20)
        yield .3
        self.check('Space jumps through input',self.p.get_actor_location().z>base+65,height=self.p.get_actor_location().z-base)
        self.key(0x20,False)
        yield 1
        for key,mode in [(0x32,1),(0x33,2),(0x34,3)]:
            self.press(key)
            yield .8
            self.check('Orthographic key '+str(mode+1),self.p.get_editor_property('view')==mode and self.p.get_editor_property('view_camera').get_editor_property('projection_mode')==unreal.CameraProjectionMode.ORTHOGRAPHIC)
            d=self.g.get_editor_property('dimensions')
            centre=self.pc.project_world_location_to_screen(unreal.Vector(d.x*100,d.y*100,d.z*100))
            w,h=self.pc.get_viewport_size()
            corners=[self.pc.project_world_location_to_screen(unreal.Vector(x,y,z)) for x in [0,d.x*200] for y in [0,d.y*200] for z in [0,d.z*200]]
            self.check('Whole-puzzle framing view '+str(mode),abs(centre.x-w/2)<5 and abs(centre.y-h/2)<5 and all(0<=v.x<=w and 0<=v.y<=h for v in corners),centre=[centre.x,centre.y],viewport=[w,h])
        self.g.restart();self.p.set_view(2)
        yield .8
        before=self.state()
        self.check('Reject player slab',not self.g.apply_cut(0,1,2))
        self.check('Reject exit slab',not self.g.apply_cut(0,10,11))
        self.check('Reject outside grid',not self.g.apply_cut(0,-1,2))
        self.check('Reject zero width',not self.g.apply_cut(0,4,4))
        self.check('Bridge cut accepted',self.g.apply_cut(0,4,9))
        self.check('Compression animates rather than teleports',any(c.get_editor_property('alive') and abs(c.get_editor_property('mesh').get_world_location().x-(c.get_editor_property('grid').x*200+100))>10 for c in self.g.get_editor_property('cells')))
        self.check('Concurrent cut rejected',not self.g.apply_cut(0,3,4))
        yield .7
        after=self.state()
        self.check('Bridge world shrinks by 5 cells',self.g.get_editor_property('dimensions').x==7)
        self.check('Geometry actually moves with collision',all(c.get_editor_property('mesh').get_world_location().distance(unreal.Vector(c.get_editor_property('grid').x*200+100,c.get_editor_property('grid').y*200+100,c.get_editor_property('grid').z*200+100))<.1 for c in self.g.get_editor_property('cells') if c.get_editor_property('alive')))
        self.press(0x5A)
        yield .7
        self.check('Z undo restores exact grid',self.state()==before)
        self.press(0x59)
        yield .7
        self.check('Y redo restores exact result',self.state()==after)
        self.press(0x31)
        yield .7
        self.check('1 restores normal camera',self.p.get_editor_property('view')==0 and self.p.get_editor_property('view_camera').get_editor_property('projection_mode')==unreal.CameraProjectionMode.PERSPECTIVE)
        # Walk the joined platforms using real character physics.
        self.walk_target=unreal.Vector(1100,500,290)
        for _ in range(150):
            pos=self.p.get_actor_location()
            delta=self.walk_target-pos;delta.z=0
            if delta.length()<75:break
            self.p.add_movement_input(delta/ max(delta.length(),.001),1)
            yield .016
        self.check('Walk across compressed bridge and reach exit',self.g.get_editor_property('complete'),position=str(self.p.get_actor_location()))
        yield 2.5
        self.check('Exit advances to puzzle 2',self.g.get_editor_property('stage')==1)
        self.g.set_editor_property('stage',1);self.g.restart();self.p.set_view(1)
        yield .8
        initial=self.state()
        self.check('Metal cut rejected',not self.g.apply_cut(2,4,5),reason=self.g.validate_cut(2,4,5))
        self.check('Rejection leaves all cells intact',self.state()==initial)
        self.check('Paper layers cut accepted',self.g.apply_cut(2,2,4))
        yield .7
        cells=self.g.get_editor_property('cells')
        self.check('Metal survives and descends',all(c.get_editor_property('alive') and c.get_editor_property('grid').z==2 for c in cells if c.get_editor_property('metal')))
        self.check('Paper actually removed',sum(not c.get_editor_property('alive') for c in cells)==40)
        self.check('Removed cells have no collision',all(c.get_editor_property('mesh').get_collision_enabled()==(unreal.CollisionEnabled.QUERY_AND_PHYSICS if c.get_editor_property('alive') else unreal.CollisionEnabled.NO_COLLISION) for c in cells))
        self.g.undo();yield .7
        self.check('Undo restores removed paper and metal',self.state()==initial)
        self.g.redo();yield .7
        self.g.set_editor_property('stage',2);self.g.restart();self.p.set_view(2);yield .8
        initial=self.state()
        self.check('Final puzzle horizontal cut',self.g.apply_cut(1,3,7));yield .7
        middle=self.state()
        self.p.set_view(1);yield .6
        self.check('Final puzzle vertical cut',self.g.apply_cut(2,2,4));yield .7
        final=self.state()
        self.g.undo();yield .6
        self.check('Undo C restores B across views',self.state()==middle)
        self.g.undo();yield .6
        self.check('Multiple undo restores original',self.state()==initial)
        self.g.redo();yield .6
        self.check('Redo intermediate state',self.state()==middle)
        self.g.redo();yield .6
        self.check('Redo final state',self.state()==final)
        self.g.undo();yield .6
        self.check('New branch cut accepted',self.g.apply_cut(2,2,3));yield .6
        branch=self.state();self.g.redo();yield .6
        self.check('New cut clears redo branch',self.state()==branch)
        self.press(0x52);yield .8
        self.check('R restarts current puzzle',self.state()==initial and self.g.get_editor_property('stage')==2)
        self.p.set_view(2)
        self.g.apply_cut(1,3,7);yield .7
        first_player=self.p.get_actor_location()
        self.p.set_actor_location(unreal.Vector(500,300,290),False,True)
        self.p.set_view(1);self.g.apply_cut(2,2,4);yield .7
        second_player=self.p.get_actor_location()
        self.g.undo();yield .7
        self.check('Undo restores position before second cut',self.p.get_actor_location().distance(second_player)<1)
        self.g.undo();yield .7
        self.g.redo();yield .7
        self.check('Redo preserves original first-cut player position',self.p.get_actor_location().distance(first_player)<1)
        self.g.redo();yield .7
        self.check('Redo preserves second-cut player position',self.p.get_actor_location().distance(second_player)<1)
        self.p.set_view(0)
        self.p.set_actor_location(unreal.Vector(-400,-400,-700),False,True)
        yield .8
        self.check('Falling recovers to safe current-state position',self.p.get_actor_location().distance(second_player)<5)
        self.g.set_editor_property('stage',0);self.g.restart()

snip_probe=SNIPProbe()
