#pragma once
#include <string>

// A scripted run for checking the renderer without anyone at the keyboard:
//
//   Stardust.exe --screenshots <folder>
//
// starts paused, takes a picture, plays for 3 simulated seconds, takes a
// picture, selects and tracks Earth, waits 3 more seconds, takes a final
// picture and quits. Comparing these pictures before and after a change is a
// quick way to spot a rendering regression.
struct ScreenshotScript {
  std::string folder;     // empty = not running a script
  int stage = 0;          // which step of the script we are on
  int framesInStage = 0;  // frames spent in the current stage
  double stageStartSimTime = 0.0;

  bool Active() const { return !folder.empty(); }
};

// Reads --screenshots <folder> from the command line, if present.
ScreenshotScript ParseScreenshotScript(int argc, char **argv);

// Saves what is currently drawn (call just before EndDrawing).
void SaveScreen(const ScreenshotScript &script, const char *fileName);
