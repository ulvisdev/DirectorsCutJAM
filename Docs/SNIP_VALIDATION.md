# SNIP implementation and validation

## Control repair after human testing

The earlier validation below missed a saved-level collision regression introduced
by the decorative backdrop. After restarting the editor, the backdrop sphere
and other decorative StaticMeshActors regained the BlockAll collision profile.
The pawn was inside that sphere, producing penetration correction and severe
sideways launch velocity. A standing-jump regression reproduced the failure.

The repair saves the NoCollision **profile**, disables actor collision, tags
decorative actors, and disables their collision again at puzzle initialization.
The level assembly scripts now persist the same settings. After a complete
editor restart, the targeted control suite passed 16/16 checks: WASD movement,
key-release stopping, mesh/capsule agreement, a vertical standing jump and
landing, nonblocking decoration, and a valid empty bridge slab.

Editing keeps the visible cursor during mouse capture. Both drag endpoints
are projected through the same current camera onto the puzzle
plane, rather than combining an old world-space start with a moving camera.
The performer protection rule remains active for slabs that actually cross it.

After the repair and another full editor restart, the combined mouse/core/course
regression passed **75/75** checks. This includes all three orthographic views,
mouse confirmation and cancellation, valid empty-space cuts, actual performer
and metal rejection, animated compression, history, and all puzzle exits.
The build succeeded. Current repair evidence is archived in `Docs/Validation/`:
`SNIP_ControlRegression.json`, `SNIP_RepairRegression.json`, and
`SNIP_ControlFixBuild.log`. These supersede the earlier validation for the
reported control and cutting failures.

Validated in the existing **DirectorsCutJAM** project with **Unreal Engine 5.8.2**
on Windows, 12 November 2026. Unreal MCP successfully inspected the open map,
started/stopped PIE, invoked editor scripts through the console, and captured
the running editor. The saved startup level is `/Game/SNIP/LV_SNIP_Prototype`.

## Saved integration

The saved map was inspected outside PIE, independently of the C++ build:

| Setting / actor | Verified value |
|---|---|
| World GameMode override | `/Script/DirectorsCutJAM.SNIPGameMode` |
| Default pawn | `/Script/DirectorsCutJAM.SNIPCharacter` |
| HUD | `/Script/DirectorsCutJAM.SNIPHUD` |
| Puzzle actor | One `SNIPPuzzle`, labelled `SNIP / Active Puzzle` |
| Start | One `PlayerStart`, labelled `SNIP / Entrance` |
| Presentation | Desk, cutting mat, paper props, lights, exposure volume, paper cyclorama |
| Project defaults | SNIP map and native GameMode |
| MCP | ModelContextProtocol and AllToolsets retained and enabled |

Inventory: `Saved/Logs/SNIP_EditorIntegration.json`.

## Live gameplay checks

| Suite | Result | What it exercised |
|---|---:|---|
| Core PIE integration | **45 / 45 passed** | Possession, HUD, keyboard movement/jump, all views and full-region framing, normal-view cut rejection, invalid bounds, player/exit protection, compression, cell/collision agreement, paper removal, metal survival/movement, undo/redo, branch clearing, exact player snapshots, restart, fall recovery, first-puzzle exit/progression |
| Mouse interaction | **16 / 16 passed** | Real drag/release/confirm/cancel input, both exposed axes in all three orthographic views, metal obstruction feedback and rejection |
| Course traversal | **14 / 14 passed** | All three solutions, walking over joined geometry, jumping onto the paper steps and lowered metal, automatic stage progression, final completion |

These are live editor-tick integration scripts, not just mathematical simulations.
Keyboard tests send Windows key messages; mouse tests move the viewport cursor
and send button messages. Traversal tests use the actual character movement,
jump, gravity, and collision systems. They do not teleport the character to
complete the routes. Test setup/restart and the explicit fall-recovery test do
place the character at controlled starting positions.

Machine-readable reports:

- `Saved/Logs/SNIP_PIE.json`
- `Saved/Logs/SNIP_Mouse.json`
- `Saved/Logs/SNIP_Course.json`

A copy of these successful reports, the saved-level inventory, and the final
compile log is retained in `Docs/Validation` so clearing Unreal's Saved folder
does not erase the validation record.

## Issues corrected during validation

- Height puzzles needed a reachable intermediate paper step after compression.
- The final puzzle initially exposed both required axes in Side view. Its final
  layout now requires X, Y, and Z cuts, forcing at least two perspectives.
- History now stores separate before/after snapshots per cut. Moving between
  cuts no longer changes the player placement of an earlier redo.
- Cuts cannot remove the last landing under the player or exit; final capsule
  overlap is checked before applying changes.
- UE orthographic auto-plane correction recentered the camera on the pawn,
  cropping the puzzle. Fixed clip planes and an explicit aspect-axis constraint
  now preserve diorama framing. Tests project the region centre and all corners
  to verify every editing view fits the active puzzle.
- The character material's skeletal-mesh usage flag was saved.
- Top-view vertical selections now orient the scissors using the screen axis.
- The front/side editing background now uses an unlit, noncolliding paper
  cyclorama for readable instructions and selection feedback.

The final C++ editor build succeeded. See `Saved/Logs/SNIP_Compile.log`.
Remaining compile diagnostics are the installed compiler preference warning,
include-order upgrade advice, and an engine-header movement-base deprecation;
no SNIP compilation errors remain. Startup logs also include Unreal's own
UnifiedError test messages before engine initialization; these are distinct
from gameplay errors. No SNIP runtime exceptions or missing gameplay references
were observed in the successful final runs.

## Practical limits and human checks

The core vertical slice is integrated and playable. A human playtest should
still assess first-time clarity, camera comfort, difficulty, and the requested
5–10 minute pacing. Automated traversal proves the routes work; it does not
establish how long a new player takes to discover them.

The implementation uses procedural materials, modular cubes, a recolored
template mannequin, and a Canvas HUD. Cut audio, paper particle scraps, bespoke
character art, packaged-build validation, and a general-purpose puzzle authoring
UI remain outside this prototype. The three layouts live in C++ and regenerate
their own components; they are not disconnected decorative level meshes.

Launch and controls: `Docs/SNIP_README.md`. No additional asset wiring or manual
compile is needed to press Play in the currently built project.
