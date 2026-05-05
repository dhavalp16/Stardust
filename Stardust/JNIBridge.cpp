#include "JNIBridge.h"

#if defined(PLATFORM_ANDROID)
#include "raymob.h"
#endif

// ── Global definitions ─────────────────────────────────────────────────
std::queue<CaptionEntry> captionQueue;
std::string currentTTSCaption = "";
float ttsCaptionTimer = 0.0f;
std::vector<KillFeedEntry> killFeed;

int CountWords(const char *s) {
  int count = 0;
  bool inWord = false;
  while (*s) {
    if (*s == ' ')
      inWord = false;
    else if (!inWord) {
      inWord = true;
      count++;
    }
    s++;
  }
  return count;
}

// ── AI Narrator JNI helpers ────────────────────────────────────────────
void RequestAINarration(const char *contextPrompt) {
#if defined(PLATFORM_ANDROID)
  jobject nativeInstance = GetNativeLoaderInstance();
  if (nativeInstance != NULL) {
    JNIEnv *env = AttachCurrentThread();
    if (env != NULL) {
      jclass nativeClass = env->GetObjectClass(nativeInstance);
      jmethodID method = env->GetMethodID(nativeClass, "requestAINarration",
                                          "(Ljava/lang/String;)V");
      if (method != NULL) {
        jstring jCtx = env->NewStringUTF(contextPrompt);
        env->CallVoidMethod(nativeInstance, method, jCtx);
        env->DeleteLocalRef(jCtx);
      }
      env->DeleteLocalRef(nativeClass);
      DetachCurrentThread();
    }
  }
#endif
}

// Poll Java for AI narration results (called each frame)
void PollAINarration() {
#if defined(PLATFORM_ANDROID)
  jobject nativeInstance = GetNativeLoaderInstance();
  if (nativeInstance != NULL) {
    JNIEnv *env = AttachCurrentThread();
    if (env != NULL) {
      jclass nativeClass = env->GetObjectClass(nativeInstance);
      jmethodID pollMethod = env->GetMethodID(nativeClass, "pollAINarration",
                                              "()Ljava/lang/String;");
      if (pollMethod != NULL) {
        jstring jResult =
            (jstring)env->CallObjectMethod(nativeInstance, pollMethod);
        if (jResult != NULL) {
          const char *text = env->GetStringUTFChars(jResult, nullptr);
          if (text != nullptr) {
            CaptionEntry entry;
            entry.displayText = text;
            entry.wordCount = CountWords(text);
            // Queue the caption — don't flush, let each sentence finish naturally
            captionQueue.push(entry);
            env->ReleaseStringUTFChars(jResult, text);
          }
          env->DeleteLocalRef(jResult);
        }
      }
      env->DeleteLocalRef(nativeClass);
      DetachCurrentThread();
    }
  }
#endif
}
