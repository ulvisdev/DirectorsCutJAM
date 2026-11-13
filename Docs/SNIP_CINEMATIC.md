# SNIP — Cinematic update

Open LV_SNIP_Prototype and press Play, then select PLAY on the title card.
Mouse clicks or Up/Down/Enter navigate menus. P opens pause in PIE; Escape is
also bound in-game but Unreal Editor may consume it to stop PIE. Controls and
Settings are available from both title and pause. Tab dismisses tutorials.

## Stability and transitions

The old camera switch disabled CharacterMovement and unconditionally resumed
in MOVE_Falling. The updated freeze suspends the movement component tick and
records velocity, movement mode and custom mode. Pure view changes preserve
that state. After an actual world edit, falling mode reacquires the changed
floor. The projection/orientation change occurs under a 0.20-second film splice.

USNIPPresentation owns one transition clock with a covered midpoint commit.
Death uses 0.42 seconds, R uses 0.32, and area progression uses 0.55. Death and
R regenerate only the current stage, restore its entrance/collision, clear
its history and return to normal view. Scene progression retains all five
existing puzzle layouts. The final exit returns to the title card.

## Minimal UI and tutorial

No persistent panels, control list, stage numbers, objective text or exit
captions remain in normal play. Editing retains grid, cursor, selection,
filled cyan destination ghosts and restrained film perforations. Ghosts are
drawn after the warm band so their colours remain separate; overlapping cells
along the hidden axis are deduplicated. Preview never alters collision.

The session tracks movement, jumping, the gap instruction card, first valid
cut and returning to play. A small jump prop belongs to a first-area module;
it follows that module through cuts/history and is considered by overlap
recovery. Walking past the prop without jumping cannot block the tutorial.
Prompts fade. Completed steps survive deaths/restarts; Tab and Settings allow
skipping. Metal, other views, undo, redo and repeated-death restart reminders
are contextual and time-limited. Progress/settings are session-scoped, not
saved across application launches.

## Art and configuration

Seven new materials live under Content/SNIP/Cinematic/Materials. Existing art
assets and the player model were not rewritten. PaperMaterial and MetalMaterial
are exposed on SNIPPuzzle for replacement. The generated paper uses a deliberate
cream/sage/teal/coral palette; separately assigned final art bypasses that tint.
Ghost/highlight materials are supplied for future mesh previews; the current
preview draws equivalent colours in Canvas and remains non-colliding.

FilmPresentation on SNIPCharacter exposes view/death/restart/scene durations,
hint thresholds and six replaceable sound references. These reuse the existing
temporary synthesized effects; no new recorded Foley or music is claimed.
Settings toggle SNIP sound events and impact camera shake.

The main and pause Controls panel reads action/axis bindings from InputSettings.
It is a native Canvas interface, not a set of unconfigured widget assets.

## Validation

MCP confirmed DirectorsCutJAM and LV_SNIP_Prototype. Unreal compiled and the
saved actor references were integrated through editor Python. Live PIE tests
cover keyboard/mouse menus, tutorial progression, repeated grounded/airborne
view changes, death/reset/history, preview immutability, and traversal of all
five stages. Final results are in Docs/Validation/SNIP_CinematicFinal.json;
the final build log is SNIP_Cinematic_Build.log. Screenshots are under
Saved/Screenshots/WindowsEditor/SNIP_Cinematic*.png.

These are automated editor checks and screenshot inspection, not a human
playthrough. Subjective animation/audio feel and packaged-build behaviour
still need a human/package playtest. No packaged executable was produced.

Recovery: Backups/SNIP_before_Cinematic_verified.zip preserves Gameplay 2.0.
Scripts/integrate_snip_cinematic.py creates only missing new material assets
and assigns them only where the corresponding puzzle material is unset.
