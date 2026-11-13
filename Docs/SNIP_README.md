# SNIP! playable prototype

Open `DirectorsCutJAM.uproject` with UE 5.8. The startup map is
`/Game/SNIP/LV_SNIP_Prototype`. Press **Play → Selected Viewport**, then click
the game viewport. Use Play rather than Simulate so the character is possessed.
The existing map already contains the puzzle actor, PlayerStart, lighting,
materials, and `SNIPGameMode` override. No Blueprint wiring step is required.

## Controls

| Input | Action |
|---|---|
| WASD / Space | Walk / jump in normal view |
| 1 | Angled perspective gameplay view |
| 2 / 3 / 4 | Front X–Z / Top X–Y / Side Y–Z orthographic editing |
| Hold LMB and drag | Choose a whole-cell band; the dominant drag direction locks its axis |
| Release, then click LMB | Lock, then execute the selected cut |
| RMB | Cancel selection |
| Z / Y | Undo / redo |
| R | Restart the current puzzle |

The mint doorway completes a puzzle. The next puzzle loads after the short
“PERFECT CUT!” message. The final puzzle stays completed until restarted.
Editing freezes the character. Compression blocks input and disables moving
collision for 0.38 seconds, then restores collision at the exact logical positions.

## Implemented architecture

- `SNIPPuzzle` owns modular cells, material rules, slab validation, animation,
  before/after snapshot history, safe recovery, and three isolated puzzle stages.
- `SNIPCharacter` owns movement, four camera modes, native input bindings,
  mouse deprojection, grid snapping, and the drag/lock/confirm interaction.
- `SNIPHUD` draws controls, graph-paper grid, valid/invalid cutting bands,
  dashed boundaries, scissors, exit labels, and completion feedback.
- `SNIPGameMode` assigns the native pawn and HUD. Both the saved map and project
  defaults reference it. The runtime module remains declared in the `.uproject`.

Each cell is a full grid cube with a logical coordinate, material classification,
alive flag, mesh, and collision. Cuts use half-open intervals `[start, end)`.
Cells before the interval stay put, selected paper cells disappear, and cells
after it translate by its width. Metal in the slab rejects the entire operation;
metal outside it moves normally. The same operation handles X, Y, and Z regardless
of which view initiated it. No runtime Boolean mesh operations or scripted
solution teleports are used.

Player/exit bounds, remaining support, and the final player capsule are checked
before mutation. History stores independent before/after states so walking
between cuts cannot overwrite an earlier redo result. A new cut discards only
the undone branch. Undo restores removed meshes, collision, logical dimensions,
metal, exit, and player position. Each puzzle has independent restart/history.

Input uses named native action/axis bindings in `DefaultInput.ini`, supported by
the project's existing Enhanced Input component classes. No missing input action
or mapping-context assets are required. Existing template assets and MCP plugins
are preserved.

## Puzzle solutions (spoilers)

Default grid spacing is 200 Unreal units; coordinates below are grid indices.

1. **The Missing Bridge:** Top or Front view, remove X `[4,9)`. Return to play
   and walk across the joined islands.
2. **Too High!:** Front or Side view, remove Z `[2,4)`. The metal platform drops
   by 400 units. Jump onto the paper step, then onto the lowered metal platform.
   Z `[4,5)` intersects metal and is rejected.
3. **Think Differently:** Top view, remove Y `[3,7)` and X `[3,7)`. Front or Side
   view, remove Z `[2,4)`. Walk along the joined L-shaped route, then jump up the
   paper step and onto the metal exit platform. All three axes are needed, so
   at least two editing views are required.

Uncut horizontal gaps are 800–1000 units; the character's approximate maximum
level-ground jump range is 515 units. Solved height steps are 200 units; jump
apex is approximately 263 units. The raised exit initially sits 600 units above
its approach step. Keep the default spacing when evaluating these tuned layouts.

## Editing and rebuilding

`SNIP / Active Puzzle` exposes Stage, Spacing, and Compression Seconds. Its
**Build Puzzle** editor action regenerates its own components for preview.
Runtime initialization regenerates the selected stage and initializes history.
The three layout definitions are in `SNIPPuzzle::BuildPuzzle`; stages share the
same rules and occupy the active region one at a time. Root translation is
supported; cuts use world-aligned axes.

Close the editor before compiling structural C++ changes:

```powershell
.\Scripts\Build-SNIP.ps1
```

`Scripts/build_snip.py` is the editor asset/map assembly script, and
`Scripts/polish_snip.py` saves the doorway material and character material usage.
They are development utilities, not required launch steps. Existing SNIP assets
are reused. The shared template content is not regenerated.

## Validation tools

Start PIE, press **2** to release mouse capture, then run one script at a time
from the editor's bottom **Cmd** field with an absolute path:

```text
py C:/Users/ulvis/Desktop/GameProjects/UnrealEngine/DirectorsCutJAM/Scripts/test_snip_pie.py
py C:/Users/ulvis/Desktop/GameProjects/UnrealEngine/DirectorsCutJAM/Scripts/test_snip_mouse.py
py C:/Users/ulvis/Desktop/GameProjects/UnrealEngine/DirectorsCutJAM/Scripts/test_snip_course.py
```

Wait for `SNIP_TEST_COMPLETE` in the Output Log before starting another script.
The tests drive the PIE world through editor tick callbacks. Keyboard and mouse
tests send real Windows messages to the Unreal window; course traversal drives
CharacterMovement and jumps without teleporting through the course. Reports are
saved in `Saved/Logs/SNIP_PIE.json`, `SNIP_Mouse.json`, and `SNIP_Course.json`.
Do not move the mouse during the mouse automation.

Human review still matters for difficulty, feel, and first-time completion time.
The prototype has procedural paper materials and a recolored mannequin; it does
not yet include bespoke character animation, cut audio, or particle scraps.
A packaged standalone build has not been validated.

## Recovering CUT TO!

`Backups/CUT_TO_before_SNIP.zip` contains the previous source, configuration,
project descriptor, map/materials, documentation, and scripts. An unpacked copy
also remains under `Saved/SNIP_Backup/Obsolete`. The old gameplay source and
`Content/CutTo` were archived rather than permanently deleted. The original
template assets, project module name, engine settings, and Unreal MCP plugins
were retained. To recover the old prototype, restore the archived files to a
separate project copy and compile that copy; do not combine two primary module
definitions in the current project's Source folder.
