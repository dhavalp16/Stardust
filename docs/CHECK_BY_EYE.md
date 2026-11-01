# What to check by eye

The automated tests cover the physics and the HDR decoder, and a scripted run
(`--screenshots`) was used to compare pictures before and after each visual
change. Interaction (mouse, keyboard, sliders) was **not** tested by a person.
Please go through this list once after building the merged result. It takes about
15 minutes. Tick each box; if something fails, note which step and what you saw.

Build the **Release** configuration and run `Stardust.exe`.

## Start-up

- [ ] 1. The window opens maximised on your 1920x1080 laptop screen, with the
      title bar visible and the help bar ("SPACE: Play/Pause | ...") visible at the
      bottom, not hidden behind the taskbar.
- [ ] 2. Start-up to first picture feels quicker than before (the sky decode went
      from about 3.4 s to 0.3 s on this laptop).
- [ ] 3. The nebula sky is visible, not a magenta checkerboard.
- [ ] 4. The Sun is bright yellow-orange and looks self-lit, with a soft orange glow
      around it. Planets are lit on the side facing the Sun and dark on the far side.
- [ ] 5. Top-left says "No planet selected". Top-right shows FPS, "Planets: 10",
      and "PAUSED" in red.
- [ ] 6. The console has no "Music file could not be opened" warning any more.

## Simulation

- [ ] 7. Press **Space**: "PLAYING" in green; planets orbit; the Sun barely moves.
- [ ] 8. Leave it playing for 2 minutes. "Planets" stays at 10 and the Moon keeps
      circling Earth (zoom in on Earth to see it).
- [ ] 9. **Stall test:** while playing, click and hold the window's title bar and
      drag the window around for 2-3 seconds, then let go. Nothing jumps; the Moon
      is still there. (Before the fixes, one long frame could knock it into Earth.)
- [ ] 10. Press **Space** again: everything freezes, including planet spin and debris.
- [ ] 11. Press **R**: everything returns to the start, paused, selection cleared.

## Camera

- [ ] 12. Hold the **right mouse button** and move the mouse: the view turns; the
      cursor is hidden while held and comes back on release.
- [ ] 13. Right mouse + **W/A/S/D/Q/E** flies forward/left/back/right/up/down.
      Far from planets it is fast; near a planet it slows down.
- [ ] 14. Right mouse + **scroll** changes fly speed. **Scroll** alone moves forward
      and back.
- [ ] 15. Looking straight up or down is limited (the view stops short of vertical
      instead of flipping).

## Selection, HUD, tracking

- [ ] 16. **Left-click** Earth: a rotating cyan reticle with "TARGET LOCKED" and
      "Earth" appears, and the top-left panel shows name, mass, velocity, two
      sliders and a TRACK PLANET button.
- [ ] 17. Click empty space: the selection clears.
- [ ] 18. **Click-through fix:** select a planet, then click the bottom edge of the
      top-left panel, just below the TRACK PLANET button (inside the dark panel). The
      selection must **not** change. (This used to select whatever was behind it.)
- [ ] 19. Clicks on the top-right panel never select a planet.
- [ ] 20. Press **TRACK PLANET**: the camera glides to Earth and follows it; the
      button says STOP TRACKING. Scroll zooms in and out. Pressing it again stops.
- [ ] 21. While tracking, press W (with right mouse held): tracking stops.
- [ ] 22. Select a different planet while tracking: tracking stops.

## Changing the physics live

- [ ] 23. Select Earth, play, and drag its **Mass** slider to the minimum: the Moon
      drifts away or crashes; nothing freezes or crashes.
- [ ] 24. Drag a planet's **Radius** slider up until it touches a neighbour: they
      merge, debris bursts out in the planet's colour and fades over about 2 s,
      "Planets" drops by one, and if the swallowed planet was selected, the
      selection clears.
- [ ] 25. Make Jupiter heavy and big and push it into the Sun: the Sun survives
      and wobbles very slightly; nothing flies off.

## Window

- [ ] 26. Un-maximise and resize the window: the 3D view keeps the right shape (not
      stretched), the top-right panel stays in the top-right corner, and the help
      bar stays at the bottom. The window will not shrink below 1024x600.
- [ ] 27. The key-press overlay (bottom right) shows the keys you press and fades.

## Tools

- [ ] 28. `ctest --preset release` reports 52 tests passed.
- [ ] 29. `build\release\stardust_bench.exe` prints four sections; its accuracy and
      stall tables match `docs/MEASUREMENTS.md` exactly (they are deterministic). The
      timing numbers will differ a little.
- [ ] 30. `build\release\Stardust.exe --screenshots C:\temp\shots` (make the folder
      first) leaves three PNGs there and closes by itself.

If anything fails, open an issue or note it in `docs/PROGRESS_LOG.md`, with the
step number.
