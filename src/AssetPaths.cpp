#include "AssetPaths.h"
#include "raylib.h"
#include <string>

namespace {

// A file that only exists inside the asset root.
const char *kMarker = "resources/shaders/glsl330/lighting.fs";

bool IsAssetRoot(const std::string &dir) {
  return FileExists((dir + "/" + kMarker).c_str());
}

} // namespace

bool ChangeToAssetRoot() {
  // Candidates, in order of preference:
  //  1. The current working directory (someone ran the app from the repo root).
  //  2. The folder holding the .exe (assets copied next to it for a release).
  //  3. The source folder this build was configured from (set by CMake), which
  //     is what makes a fresh build run straight out of build/<preset>/.
  std::string candidates[] = {
      GetWorkingDirectory(),
      GetApplicationDirectory(),
#ifdef STARDUST_SOURCE_DIR
      STARDUST_SOURCE_DIR,
#endif
  };

  for (const std::string &dir : candidates) {
    if (IsAssetRoot(dir)) {
      ChangeDirectory(dir.c_str());
      TraceLog(LOG_INFO, "STARDUST: Asset root is %s", dir.c_str());
      return true;
    }
  }
  TraceLog(LOG_WARNING, "STARDUST: Could not find '%s' in any asset root candidate", kMarker);
  return false;
}
