import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
scope={}
exec(compile((root/'Scripts/test_snip_control_regression.py').read_text().split('snip_control_probe=ControlProbe()')[0],'controls','exec'),scope)
class HistoryProbe(scope['ControlProbe']):
    report_name='SNIP_Gameplay2History'
    def run(self):
        yield from super().run()
        self.g.set_editor_property('stage',2);self.g.restart();self.p.set_view(2);yield .8
        initial=self.state();states=[]
        for axis,lo,hi in [(1,3,7),(0,3,7),(2,2,4)]:
            self.check('History cut accepted '+str(axis),self.g.apply_cut(axis,lo,hi));yield .7
            states.append(self.state())
        for i,expected in enumerate([states[1],states[0],initial]):
            self.g.undo();yield .4
            self.check('Multi undo exact '+str(i),self.state()==expected)
        for i,expected in enumerate(states):
            self.g.redo();yield .5
            self.check('Multi redo exact '+str(i),self.state()==expected)
        self.g.set_editor_property('stage',0);self.g.restart();self.p.set_view(2);yield .7
        self.check('Overlap recovery fixture cut accepted',self.g.apply_cut(0,1,2));yield .7
        self.p.set_actor_location(unreal.Vector(300,300,190),False,False)
        self.g.undo();yield .4
        pos=self.p.get_actor_location()
        self.check('Undo overlap resolves locally above paper',pos.z>=288 and pos.distance(unreal.Vector(300,300,190))<150,position=str(pos))
        self.p.set_view(0);yield .8
        self.check('Recovered capsule stands without collision ejection',self.p.get_actor_location().distance(pos)<5)
        self.g.set_editor_property('stage',0);self.g.restart();self.p.set_view(2);yield .8
        self.g.apply_cut(0,4,9)
        self.g.undo();self.g.redo();self.g.restart();yield .7
        self.check('History and reset cannot corrupt active animation',self.g.get_editor_property('dimensions').x==7)
        self.g.restart();self.p.set_view(2);yield .8
        mesh=next(c.get_editor_property('mesh') for c in self.g.get_editor_property('cells') if c.get_editor_property('grid').x==9)
        self.g.apply_cut(0,4,9)
        yield .12
        self.check('Anticipation moves opposite compression',mesh.get_world_location().x>1901,position=str(mesh.get_world_location()))
        yield .08
        a=mesh.get_world_location().x
        yield .06
        b=mesh.get_world_location().x
        yield .06
        c=mesh.get_world_location().x
        self.check('Compression accelerates toward impact',b-c>a-b,early_distance=a-b,late_distance=b-c)
        yield .3
        self.check('Impact ends exactly on grid',abs(mesh.get_world_location().x-900)<.01)
        self.g.restart();self.p.set_view(0)
snip2_history=HistoryProbe()
