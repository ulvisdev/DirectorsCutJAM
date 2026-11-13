"""Run the mouse, core and course checks sequentially in one cold-start PIE session."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())/'Scripts'
base={};mouse={};course={}
exec((root/'test_snip_pie.py').read_text().split('snip_probe=SNIPProbe()')[0],base)
exec((root/'test_snip_mouse.py').read_text().split('snip_mouse_probe=MouseProbe()')[0],mouse)
exec((root/'test_snip_course.py').read_text().split('snip_course_probe=CourseProbe()')[0],course)
MouseClass=mouse['MouseProbe'];CourseClass=course['CourseProbe']
class RepairProbe(base['SNIPProbe']):
    report_name='SNIP_RepairRegression'
    mouse=MouseClass.mouse
    button=MouseClass.button
    walk=CourseClass.walk
    def run(self):
        yield from mouse['MouseProbe'].run(self)
        yield from base['SNIPProbe'].run(self)
        yield from course['CourseProbe'].run(self)
snip_repair_probe=RepairProbe()
