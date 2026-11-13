# SNIP Gameplay 2.0

Upgrades the existing native pawn, puzzle actor and HUD in LV_SNIP_Prototype.
The three original courses remain; Trapdoor and Riding the Seam follow them.
No template assets or MCP plugins were removed.

## Interaction and simulation

- Drag/release immediately cuts. Grid selection uses floor-based inclusive
  endpoint tiles, a 7-pixel accidental-drag threshold, and 1.25 axis hysteresis.
- The original world-space clicked tile remains the anchor. RMB cancels and
  suppresses the release. Cutting remains restricted to orthographic views.
- Player/support intersection no longer rejects a cut. Coordinates inside a
  slab collapse to the seam; surviving support displacement carries the player.
- Editing saves velocity; animation freezes movement and capsule collision.
  Restoring gameplay resumes velocity under CharacterMovement gravity. Safe
  placement tests conservative capsule bounds against the resulting modular grid.
- Undo/redo restores world snapshots, preserves the current player location
  where clear, and searches cell-face candidates for nearby overlap recovery.
  History is blocked during animation. Restart resets only the current stage.
- Metal, exit bounds and exit support still reject intersecting cuts.

## Feedback

Default timing: 0.08s snip, 0.08s anticipation, 0.20s cubic acceleration,
0.07s impact. Undo takes 0.20s; redo uses 65% of the cut timing. Parameters
are exposed on SNIPPuzzle. Collision is disabled during interpolation and
restored only after exact grid transforms have been applied.

Scissors react to drag/release/rejection. Warm selection highlights, ghost
destination outlines, cooler editing saturation, impact camera vibration and
screen-space paper flecks accompany compression. Ghosts share the coordinate
mapping used by the cut. They never alter collision.

Six generated, deterministic temporary audio assets are imported under
Content/SNIP/Audio. These are synthesized placeholders, not recorded Foley.
Subjective sound quality and game feel still warrant a human playtest. The
paper flecks are HUD effects rather than persistent 3D debris.

## Validation and recovery

MCP verified the open DirectorsCutJAM project and LV_SNIP_Prototype, then started
a real possessed Play-In-Editor session. The native Editor target compiled.
Reports and build output are in Docs/Validation/SNIP_Gameplay2*.json/log.
Final results: **52/52 gameplay checks and 32/32 movement/history/animation
checks passed**. The frame-rate checks measured ~0.03333, ~0.01667 and
~0.00927 seconds per frame at 30/60/120 caps. All five exits were reached using
CharacterMovement traversal; new-course traversal does not teleport the pawn.
Scripts/test_snip2.py exercises real viewport mouse events and five physical
course traversals. Scripts/test_snip2_history.py exercises keyboard movement,
multi-step history, overlap recovery, and compression timing. Test scripts
modify only the PIE world and finish at stage 0.

The initial fifth-stage test selected Z [1,4), which correctly hit the protected
exit. The validated route uses Z [2,4), followed by Y [3,9). An overlap test
fixture initially selected metal in stage 3; it was corrected to use the first
paper-only stage before evaluating recovery.

Pre-change backup: Backups/SNIP_before_Gameplay2_verified.zip.
SHA256: 70869BD81A232F3E84AA050A6CF5C31D222501E85137B7645DA7230E3F96E042.
Rebuild with Scripts/Build-SNIP.ps1 while Unreal is closed. Normal play requires
no scripts: open the existing prototype map and use Play, not Simulate.
