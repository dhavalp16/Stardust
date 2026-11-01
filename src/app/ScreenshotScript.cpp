#include "ScreenshotScript.h"
#include "raylib.h"
#include "rlgl.h"
#include <string_view>

ScreenshotScript ParseScreenshotScript(int argc, char **argv) {
  ScreenshotScript script;
  for (int i = 1; i + 1 < argc; i++) {
    if (std::string_view(argv[i]) == "--screenshots") script.folder = argv[i + 1];
  }
  return script;
}

void SaveScreen(const ScreenshotScript &script, const char *fileName) {
  // raylib queues 2D drawing (the HUD) until EndDrawing; flush it first so the
  // picture includes everything.
  rlDrawRenderBatchActive();

  // Read back exactly the framebuffer. (raylib's LoadImageFromScreen and
  // TakeScreenshot multiply by the Windows display scale, which on a 125%
  // display reads past the edge of the image and leaves black bands.)
  Image image = {};
  image.width = GetRenderWidth();
  image.height = GetRenderHeight();
  image.mipmaps = 1;
  image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
  image.data = rlReadScreenPixels(image.width, image.height);
  std::string path = script.folder + "/" + fileName;
  ExportImage(image, path.c_str());
  UnloadImage(image);
}
