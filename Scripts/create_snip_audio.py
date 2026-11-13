"""Generate deterministic temporary Foley-like tones, then import in Unreal Python."""
import math, random, struct, wave
from pathlib import Path
root=Path(__file__).resolve().parents[1]
folder=root/'Saved'/'SNIPAudio'
folder.mkdir(exist_ok=True)
for name,duration,freq,noise in [('Snip',.09,1800,.8),('Slide',.36,160,.9),('Impact',.09,95,.35),('Reject',.16,2100,.12),('Undo',.20,600,.5),('Redo',.06,1300,.65)]:
    rng=random.Random(42);samples=[];rate=22050
    for i in range(int(duration*rate)):
        t=i/rate;u=t/duration
        envelope=math.sin(math.pi*min(1,u*12))*math.exp(-u*4) if name!='Slide' else math.sin(math.pi*u)**2*.4
        tone=math.sin(2*math.pi*freq*t*(1-u*.35))
        value=(tone*(1-noise)+rng.uniform(-1,1)*noise)*envelope*.65
        samples.append(struct.pack('<h',int(value*32767)))
    with wave.open(str(folder/('SNIP_'+name+'.wav')),'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate);f.writeframes(b''.join(samples))
try:
    import unreal
except ImportError:
    pass
else:
    tasks=[]
    for wav in folder.glob('*.wav'):
        task=unreal.AssetImportTask()
        task.set_editor_property('filename',str(wav))
        task.set_editor_property('destination_path','/Game/SNIP/Audio')
        task.set_editor_property('automated',True)
        task.set_editor_property('replace_existing',True)
        task.set_editor_property('save',True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
