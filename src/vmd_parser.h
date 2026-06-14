#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <windows.h>


struct VmdBoneKeyframe {
  std::string boneName;   
  uint32_t frame;
  float pos[3];           
  float rot[4];           
  uint8_t interp[64];     
};

struct VmdBoneTimeline {
  std::string boneName;
  std::vector<VmdBoneKeyframe> keys; 
};

struct VmdMorphKeyframe {
  std::string morphName;  
  uint32_t frame;
  float weight;           
};

struct VmdMorphTimeline {
  std::string morphName;
  std::vector<VmdMorphKeyframe> keys; 
  
  float Sample(float frameF) const {
    if (keys.empty()) return 0;
    if (frameF <= keys.front().frame) return keys.front().weight;
    if (frameF >= keys.back().frame) return keys.back().weight;
    int lo = 0, hi = (int)keys.size() - 1;
    while (lo < hi - 1) {
      int mid = (lo + hi) / 2;
      if (keys[mid].frame <= frameF) lo = mid;
      else hi = mid;
    }
    float t = (frameF - keys[lo].frame) / (float)(keys[hi].frame - keys[lo].frame);
    return keys[lo].weight + (keys[hi].weight - keys[lo].weight) * t;
  }
};

struct VmdCameraKeyframe {
  uint32_t frame;
  float distance;       
  float position[3];    
  float rotation[3];    
  uint8_t interp[24];   
  uint32_t fov;         
  uint8_t perspective;  
};

struct VmdFile {
  char signature[31];
  char modelName[21];     
  uint32_t totalFrames;   

  std::map<std::string, VmdBoneTimeline> boneTimelines;
  std::map<std::string, VmdMorphTimeline> morphTimelines;
  std::vector<VmdCameraKeyframe> cameraKeys;

  bool loaded;
  std::string error;

  VmdFile() : totalFrames(0), loaded(false) {
    memset(signature, 0, sizeof(signature));
    memset(modelName, 0, sizeof(modelName));
  }
};

static std::string SjisToUtf8(const char *sjis, int maxLen) {
  int sjisLen = 0;
  while (sjisLen < maxLen && sjis[sjisLen] != '\0') sjisLen++;
  if (sjisLen == 0) return "";

  int wLen = MultiByteToWideChar(932, 0, sjis, sjisLen, NULL, 0);
  if (wLen <= 0) return "";
  std::vector<wchar_t> wBuf(wLen);
  MultiByteToWideChar(932, 0, sjis, sjisLen, wBuf.data(), wLen);

  int u8Len = WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), wLen, NULL, 0, NULL, NULL);
  if (u8Len <= 0) return "";
  std::string result(u8Len, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), wLen, &result[0], u8Len, NULL, NULL);
  return result;
}

static VmdFile *LoadVmd(const char *path) {
  VmdFile *vmd = new VmdFile();

  FILE *f = fopen(path, "rb");
  if (!f) {
    vmd->error = "Cannot open file";
    return vmd;
  }

  fseek(f, 0, SEEK_END);
  long fileSize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char sigBuf[30];
  if (fread(sigBuf, 1, 30, f) != 30) {
    vmd->error = "File too small for header";
    fclose(f); return vmd;
  }
  memcpy(vmd->signature, sigBuf, 30);
  vmd->signature[30] = '\0';

  if (strncmp(sigBuf, "Vocaloid Motion Data 0002", 25) != 0 &&
      strncmp(sigBuf, "Vocaloid Motion Data file", 25) != 0) {
    vmd->error = "Invalid VMD signature";
    fclose(f); return vmd;
  }

  char modelBuf[20];
  if (fread(modelBuf, 1, 20, f) != 20) {
    vmd->error = "File too small for model name";
    fclose(f); return vmd;
  }
  std::string modelUtf8 = SjisToUtf8(modelBuf, 20);
  strncpy(vmd->modelName, modelUtf8.c_str(), 20);
  vmd->modelName[20] = '\0';

  uint32_t boneKeyCount = 0;
  if (fread(&boneKeyCount, 4, 1, f) != 1) {
    vmd->error = "Cannot read bone keyframe count";
    fclose(f); return vmd;
  }

  uint32_t maxFrame = 0;

  for (uint32_t i = 0; i < boneKeyCount; i++) {
    char nameBuf[15];
    uint32_t frame;
    float pos[3];
    float rot[4];
    uint8_t interp[64];

    if (fread(nameBuf, 1, 15, f) != 15 ||
        fread(&frame, 4, 1, f) != 1 ||
        fread(pos, 4, 3, f) != 3 ||
        fread(rot, 4, 4, f) != 4 ||
        fread(interp, 1, 64, f) != 64) {
      vmd->error = "Truncated bone keyframe at index " + std::to_string(i);
      fclose(f); return vmd;
    }

    VmdBoneKeyframe key;
    key.boneName = SjisToUtf8(nameBuf, 15);
    key.frame = frame;
    memcpy(key.pos, pos, sizeof(pos));
    memcpy(key.rot, rot, sizeof(rot));
    memcpy(key.interp, interp, sizeof(interp));

    if (frame > maxFrame) maxFrame = frame;

    auto &tl = vmd->boneTimelines[key.boneName];
    if (tl.boneName.empty()) tl.boneName = key.boneName;
    tl.keys.push_back(key);
  }

  vmd->totalFrames = maxFrame;

  for (auto &pair : vmd->boneTimelines) {
    std::sort(pair.second.keys.begin(), pair.second.keys.end(),
      [](const VmdBoneKeyframe &a, const VmdBoneKeyframe &b) {
        return a.frame < b.frame;
      });
  }

  uint32_t morphKeyCount = 0;
  if (fread(&morphKeyCount, 4, 1, f) == 1 && morphKeyCount > 0) {
    for (uint32_t i = 0; i < morphKeyCount; i++) {
      char nameBuf[15];
      uint32_t frame;
      float weight;
      
      if (fread(nameBuf, 1, 15, f) != 15 ||
          fread(&frame, 4, 1, f) != 1 ||
          fread(&weight, 4, 1, f) != 1) {
        break;
      }
      
      VmdMorphKeyframe key;
      key.morphName = SjisToUtf8(nameBuf, 15);
      key.frame = frame;
      key.weight = weight;
      
      if (frame > maxFrame) maxFrame = frame;
      
      auto &tl = vmd->morphTimelines[key.morphName];
      if (tl.morphName.empty()) tl.morphName = key.morphName;
      tl.keys.push_back(key);
    }
    
    for (auto &pair : vmd->morphTimelines) {
      std::sort(pair.second.keys.begin(), pair.second.keys.end(),
        [](const VmdMorphKeyframe &a, const VmdMorphKeyframe &b) {
          return a.frame < b.frame;
        });
    }
    
    vmd->totalFrames = maxFrame; 
  }

  uint32_t cameraKeyCount = 0;
  if (fread(&cameraKeyCount, 4, 1, f) == 1 && cameraKeyCount > 0 &&
      cameraKeyCount < 1000000) {
    for (uint32_t i = 0; i < cameraKeyCount; i++) {
      uint32_t frame;
      float distance;
      float pos[3];
      float rot[3];
      uint8_t interp[24];
      uint32_t fov;
      uint8_t perspective;

      if (fread(&frame, 4, 1, f) != 1 ||
          fread(&distance, 4, 1, f) != 1 ||
          fread(pos, 4, 3, f) != 3 ||
          fread(rot, 4, 3, f) != 3 ||
          fread(interp, 1, 24, f) != 24 ||
          fread(&fov, 4, 1, f) != 1 ||
          fread(&perspective, 1, 1, f) != 1) {
        break; 
      }

      VmdCameraKeyframe key;
      key.frame = frame;
      key.distance = distance;
      memcpy(key.position, pos, sizeof(pos));
      memcpy(key.rotation, rot, sizeof(rot));
      memcpy(key.interp, interp, sizeof(interp));
      key.fov = fov;
      key.perspective = perspective;

      if (frame > maxFrame) maxFrame = frame;
      vmd->cameraKeys.push_back(key);
    }

    std::sort(vmd->cameraKeys.begin(), vmd->cameraKeys.end(),
      [](const VmdCameraKeyframe &a, const VmdCameraKeyframe &b) {
        return a.frame < b.frame;
      });

    vmd->totalFrames = maxFrame;
  }

  fclose(f);
  vmd->loaded = true;
  return vmd;
}

static void FreeVmd(VmdFile *vmd) {
  if (vmd) delete vmd;
}

static void DumpVmd(const VmdFile *vmd, FILE *out) {
  fprintf(out, "=== VMD File Info ===\n");
  fprintf(out, "Signature: %.30s\n", vmd->signature);
  fprintf(out, "Model: %s\n", vmd->modelName);
  fprintf(out, "Loaded: %s\n", vmd->loaded ? "YES" : "NO");
  if (!vmd->loaded) {
    fprintf(out, "Error: %s\n", vmd->error.c_str());
    return;
  }

  fprintf(out, "Total frames: %u (%.1f sec @ 30fps)\n",
    vmd->totalFrames, vmd->totalFrames / 30.0f);
  fprintf(out, "Bone timelines: %zu\n", vmd->boneTimelines.size());
  fprintf(out, "Morph timelines: %zu\n\n", vmd->morphTimelines.size());

  for (const auto &pair : vmd->boneTimelines) {
    const auto &tl = pair.second;
    fprintf(out, "  [BONE] %s: %zu keys", tl.boneName.c_str(), tl.keys.size());
    if (!tl.keys.empty()) {
      fprintf(out, " (frames %u-%u)", tl.keys.front().frame, tl.keys.back().frame);
    }
    fprintf(out, "\n");
  }
  
  for (const auto &pair : vmd->morphTimelines) {
    const auto &tl = pair.second;
    fprintf(out, "  [MORPH] %s: %zu keys", tl.morphName.c_str(), tl.keys.size());
    if (!tl.keys.empty()) {
      fprintf(out, " (frames %u-%u, peak=%.2f)", tl.keys.front().frame, tl.keys.back().frame,
        [&]() { float m=0; for(auto&k:tl.keys) if(k.weight>m) m=k.weight; return m; }());
    }
    fprintf(out, "\n");
  }
}
