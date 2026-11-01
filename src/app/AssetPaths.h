#pragma once

// Every asset path in the app is relative ("assets/earth.glb"), so the current
// working directory must be the folder that contains `assets/` and `resources/`.
// This finds that folder and switches to it, so the app works no matter where
// it was started from (double-click, Visual Studio, a terminal in any folder).
//
// Returns true if the folder was found. On false the app still runs, but
// models, shaders and the skybox will be missing.
bool ChangeToAssetRoot();
