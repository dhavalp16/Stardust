#include "Debris.h"
#include "Convert.h"
#include "raymath.h"

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
