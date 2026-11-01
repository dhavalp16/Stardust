#include "Debris.h"
#include "Convert.h"
#include "raymath.h"
#include <algorithm>

void SpawnDebris(const sim::MergeEvent &event, Color color, std::vector<Fragment> &fragments) {
  Vector3 victimVelocity = ToRaylib(event.victimVelocity);
  int count = 30 + GetRandomValue(0, 20);
  for (int f = 0; f < count; f++) {
    Vector3 randDir = {(float)GetRandomValue(-100, 100) / 100.0f,
                       (float)GetRandomValue(-100, 100) / 100.0f,
                       (float)GetRandomValue(-100, 100) / 100.0f};
    randDir = Vector3Normalize(randDir);

    float speed = (float)GetRandomValue(2, 8) + Vector3Length(victimVelocity) * 0.15f;
    Vector3 baseVelocity = Vector3Scale(victimVelocity, 0.4f);

    Fragment frag;
    frag.position = ToRaylib(event.victimPosition);
    frag.velocity = Vector3Add(baseVelocity, Vector3Scale(randDir, speed));
    frag.size = event.victimRadius * ((float)GetRandomValue(10, 30) / 100.0f);
    frag.life = 1.0f;
    frag.color = color;
    frag.isAlive = true;
    fragments.push_back(frag);
  }
}

void UpdateDebris(std::vector<Fragment> &fragments, float dt) {
  for (Fragment &frag : fragments) {
    frag.position = Vector3Add(frag.position, Vector3Scale(frag.velocity, dt));
    frag.life -= dt * 0.5f; // fully faded after 2 seconds
    if (frag.life <= 0.0f) frag.isAlive = false;
  }
  // "Erase-remove idiom": remove_if moves the survivors to the front and
  // returns where the dead ones start; erase then chops those off the end.
  fragments.erase(std::remove_if(fragments.begin(), fragments.end(),
                                 [](const Fragment &f) { return !f.isAlive; }),
                  fragments.end());
}

void DrawDebris(const std::vector<Fragment> &fragments) {
  for (const Fragment &frag : fragments) {
    Color color = frag.color;
    color.a = static_cast<unsigned char>(frag.life * 255.0f);
    DrawCubeV(frag.position, Vector3{frag.size, frag.size, frag.size}, color);
  }
}
