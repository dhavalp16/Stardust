#pragma once
#include "Planet.h"
#include "JNIBridge.h"
#include "Collision.h"
#include "raylib.h"
#include <vector>

// Core engine states
enum EngineState { PAUSED, PLAYING, SUMMARY };

// Draw the selection reticle around the selected planet
void DrawSelectionReticle(Planet *selectedPlanet, Camera3D &camera);

// Draw the planet controls panel (mass/radius sliders, track, reset)
void DrawControlsPanel(int screenWidth, int screenHeight,
                       Planet *&selectedPlanet, bool &isTracking,
                       EngineState &currentState,
                       std::vector<Planet> &activePlanets,
                       std::vector<Planet> &initialPlanets,
                       std::vector<Fragment> &activeFragments,
                       Planet *&prevSelectedPlanet,
                       float &prevSliderMass, float &prevSliderRadius,
                       float &settledMass, float &toastTimer);

// Draw the status panel (play/pause indicator + camera speed slider)
void DrawStatusPanel(int screenWidth, EngineState currentState,
                     float &cameraSpeed);

// Process toast notifications for slider changes and AI narration triggers
void ProcessToastAndNarration(int screenWidth, int screenHeight,
                              Planet *selectedPlanet,
                              Planet *&prevSelectedPlanet,
                              std::vector<Planet> &activePlanets,
                              float &prevSliderMass, float &prevSliderRadius,
                              float &settledMass, float &toastTimer,
                              char *toastText, size_t toastTextSize,
                              float dt);

// Draw toast notification
void DrawToast(int screenWidth, float &toastTimer, char *toastText, float dt);

// Draw AI narrator subtitle captions
void DrawCaptions(int screenWidth, int screenHeight, float dt);

// Draw the kill feed log on the right side
void DrawKillFeed(int screenWidth, float dt);

// Draw the play/pause overlay button
void DrawPlayPauseButton(int screenWidth, int screenHeight,
                         EngineState &currentState);

// Draw the help text bar at the bottom
void DrawHelpBar(int screenHeight);
