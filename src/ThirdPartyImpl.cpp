// Single-header libraries (rlights.h, raygui.h) contain their whole
// implementation inside the header, guarded by a macro. Exactly one .cpp file
// in the program must define that macro before including the header, or the
// linker finds no definitions (none defined) or duplicates (more than one).
// Keeping that in its own file keeps main.cpp about our code only.
#include "raylib.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
