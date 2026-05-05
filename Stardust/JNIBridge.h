#pragma once
#include <queue>
#include <vector>
#include <string>

// Caption entry for subtitle display
struct CaptionEntry {
  std::string displayText;
  int wordCount;
};

// Kill-feed log for collisions (like FPS games)
struct KillFeedEntry {
  std::string survivorName;
  std::string victimName;
  float timer; // seconds remaining to display
};

// Globals shared with main loop
extern std::queue<CaptionEntry> captionQueue;
extern std::string currentTTSCaption;
extern float ttsCaptionTimer;
extern std::vector<KillFeedEntry> killFeed;

// Word count utility
int CountWords(const char *s);

// JNI bridge functions
void RequestAINarration(const char *contextPrompt);
void PollAINarration();
