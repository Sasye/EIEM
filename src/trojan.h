#pragma once




















static bool s_poseReady;
static Il2CppHumanPose s_cachedPose;
static float *s_musclePtr;
static float s_savedIdleMuscles[128] =
    {}; 
static float s_savedIdleBodyPos[3] = {};
static float s_savedIdleBodyRot[4] = {0, 0, 0, 1};
static int s_actualMuscleCount = 95; 




static int g_stdToGameMap[95];
static bool g_dynamicMapReady = false;


static void InitHardcodedMuscleMap() {
  for (int i = 0; i < 95; i++) {
    if (i <= 28)
      g_stdToGameMap[i] = i;
    else if (i <= 36)
      g_stdToGameMap[i] = i + 3;
    else
      g_stdToGameMap[i] = i + 6;
  }
}


static const char *g_muscleNames[] = {
    "Spine Front-Back",      
    "Spine Left-Right",      
    "Spine Twist L-R",       
    "Chest Front-Back",      
    "Chest Left-Right",      
    "Chest Twist L-R",       
    "UpperChest Front-Back", 
    "UpperChest Left-Right", 
    "UpperChest Twist L-R",  
    "Neck Nod Down-Up",      
    "Neck Tilt L-R",         
    "Neck Turn L-R",         
    "Head Nod Down-Up",      
    "Head Tilt L-R",         
    "Head Turn L-R",         
    "Left Eye Down-Up",      
    "Left Eye In-Out",       
    "Right Eye Down-Up",     
    "Right Eye In-Out",      
    "Jaw Close",             
    "Jaw Left-Right",        
    "L UpperLeg Front-Back", 
    "L UpperLeg In-Out",     
    "L UpperLeg Twist",      
    "L LowerLeg Stretch",    
    "L LowerLeg Twist",      
    "L Foot Up-Down",        
    "L Foot Twist",          
    "L Toes Up-Down",        
    "R UpperLeg Front-Back", 
    "R UpperLeg In-Out",     
    "R UpperLeg Twist",      
    "R LowerLeg Stretch",    
    "R LowerLeg Twist",      
    "R Foot Up-Down",        
    "R Foot Twist",          
    "R Toes Up-Down",        
    "L Shoulder Down-Up",    
    "L Shoulder Front-Back", 
    "L Arm Down-Up",         
    "L Arm Front-Back",      
    "L Arm Twist",           
    "L Forearm Stretch",     
    "L Forearm Twist",       
    "L Hand Down-Up",        
    "L Hand In-Out",         
    "R Shoulder Down-Up",    
    "R Shoulder Front-Back", 
    "R Arm Down-Up",         
    "R Arm Front-Back",      
    "R Arm Twist",           
    "R Forearm Stretch",     
    "R Forearm Twist",       
    "R Hand Down-Up",        
    "R Hand In-Out",         
    "LF Thumb1 Stretch",     
    "LF Thumb Spread",       
    "LF Thumb2 Stretch",     
    "LF Thumb3 Stretch",     
    "LF Index1 Stretch",     
    "LF Index Spread",       
    "LF Index2 Stretch",     
    "LF Index3 Stretch",     
    "LF Middle1 Stretch",    
    "LF Middle Spread",      
    "LF Middle2 Stretch",    
    "LF Middle3 Stretch",    
    "LF Ring1 Stretch",      
    "LF Ring Spread",        
    "LF Ring2 Stretch",      
    "LF Ring3 Stretch",      
    "LF Little1 Stretch",    
    "LF Little Spread",      
    "LF Little2 Stretch",    
    "LF Little3 Stretch",    
    "RF Thumb1 Stretch",     
    "RF Thumb Spread",       
    "RF Thumb2 Stretch",     
    "RF Thumb3 Stretch",     
    "RF Index1 Stretch",     
    "RF Index Spread",       
    "RF Index2 Stretch",     
    "RF Index3 Stretch",     
    "RF Middle1 Stretch",    
    "RF Middle Spread",      
    "RF Middle2 Stretch",    
    "RF Middle3 Stretch",    
    "RF Ring1 Stretch",      
    "RF Ring Spread",        
    "RF Ring2 Stretch",      
    "RF Ring3 Stretch",      
    "RF Little1 Stretch",    
    "RF Little Spread",      
    "RF Little2 Stretch",    
    "RF Little3 Stretch",    
};





static void BuildDynamicMuscleMap() {
  
  void *domain = il2cpp_domain_get();
  if (!domain) {
    Log("[MUSCLE-MAP] No domain, keeping hardcoded map");
    return;
  }
  size_t ac = 0;
  void **asms = il2cpp_domain_get_assemblies(domain, &ac);
  if (!asms || ac == 0) {
    Log("[MUSCLE-MAP] No assemblies, keeping hardcoded map");
    return;
  }

  
  void *htClass = FindClass("UnityEngine", "HumanTrait", asms, ac);
  if (!htClass) {
    Log("[MUSCLE-MAP] HumanTrait class not found, keeping hardcoded map");
    return;
  }

  
  void *getMuscleNameMethod = FindMethod(htClass, "get_MuscleName", 0);
  if (!getMuscleNameMethod) {
    Log("[MUSCLE-MAP] get_MuscleName not found, keeping hardcoded map");
    return;
  }

  
  void *exc = nullptr;
  void *nameArray = il2cpp_runtime_invoke(getMuscleNameMethod, nullptr, nullptr, &exc);
  if (exc || !nameArray) {
    Log("[MUSCLE-MAP] get_MuscleName() failed: exc=%p ret=%p", exc, nameArray);
    return;
  }

  
  
  
  int gameMuscleCnt = 0;
  __try {
    gameMuscleCnt = *(int *)((char *)nameArray + 24);
  } __except (1) {
    Log("[MUSCLE-MAP] Failed to read array length");
    return;
  }

  Log("[MUSCLE-MAP] HumanTrait.MuscleName has %d entries (game muscles=%d)",
      gameMuscleCnt, s_actualMuscleCount);

  if (gameMuscleCnt < 95) {
    Log("[MUSCLE-MAP] Game has fewer than 95 muscles (%d), keeping hardcoded map",
        gameMuscleCnt);
    return;
  }

  
  
  void **elements = (void **)((char *)nameArray + 32);
  char gameNames[256][64] = {};
  int maxRead = (gameMuscleCnt > 255) ? 255 : gameMuscleCnt;

  for (int i = 0; i < maxRead; i++) {
    __try {
      void *strObj = elements[i];
      if (strObj)
        ReadStr(strObj, gameNames[i], 64);
      else
        gameNames[i][0] = 0;
    } __except (1) {
      gameNames[i][0] = 0;
    }
  }

  
  for (int i = 0; i < maxRead; i++) {
    Log("[MUSCLE-MAP]   game[%d] = \"%s\"%s", i, gameNames[i],
        i >= 95 ? " (EXTRA)" : "");
  }

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  auto NormalizeMuscle = [](const char *src, char *dst, int dstSz) {
    
    char lower[128] = {};
    int li = 0;
    for (int i = 0; src[i] && li < 126; i++)
      lower[li++] = (src[i] >= 'A' && src[i] <= 'Z') ? (src[i] + 32) : src[i];
    lower[li] = 0;

    
    
    char *s = lower;
    int di = 0;

    while (*s && di < dstSz - 1) {
      
      if (strncmp(s, "left-right", 10) == 0) {
        dst[di++] = 'l'; dst[di++] = 'r';
        s += 10; continue;
      }
      
      if (strncmp(s, "l-r", 3) == 0) {
        dst[di++] = 'l'; dst[di++] = 'r';
        s += 3; continue;
      }
      
      if (strncmp(s, "stretched", 9) == 0) {
        memcpy(dst + di, "stretch", 7); di += 7;
        s += 9; continue;
      }
      
      if (strncmp(s, "left ", 5) == 0 && (s == lower || *(s - 1) == ' ')) {
        dst[di++] = 'l'; dst[di++] = ' ';
        s += 5; continue;
      }
      
      if (strncmp(s, "right ", 6) == 0 && (s == lower || *(s - 1) == ' ')) {
        dst[di++] = 'r'; dst[di++] = ' ';
        s += 6; continue;
      }
      
      if (strncmp(s, "upper leg", 9) == 0) {
        memcpy(dst + di, "upperleg", 8); di += 8;
        s += 9; continue;
      }
      
      if (strncmp(s, "lower leg", 9) == 0) {
        memcpy(dst + di, "lowerleg", 8); di += 8;
        s += 9; continue;
      }
      
      if (strncmp(s, "upper chest", 11) == 0) {
        memcpy(dst + di, "upperchest", 10); di += 10;
        s += 11; continue;
      }
      
      if (strncmp(s, "lf ", 3) == 0 && (s == lower || *(s - 1) == ' ')) {
        dst[di++] = 'l'; dst[di++] = ' ';
        s += 3; continue;
      }
      
      if (strncmp(s, "rf ", 3) == 0 && (s == lower || *(s - 1) == ' ')) {
        dst[di++] = 'r'; dst[di++] = ' ';
        s += 3; continue;
      }
      
      if (*s == ' ' && s[1] >= '0' && s[1] <= '9') {
        s++; continue;
      }
      
      if (strncmp(s, "twist in-out", 12) == 0) {
        memcpy(dst + di, "twist", 5); di += 5;
        s += 12; continue;
      }
      
      if (strncmp(s, "twist roll", 10) == 0) {
        memcpy(dst + di, "twist", 5); di += 5;
        s += 10; continue;
      }
      dst[di++] = *s++;
    }
    dst[di] = 0;
  };

  
  char gameNorm[256][64] = {};
  for (int i = 0; i < maxRead; i++) {
    NormalizeMuscle(gameNames[i], gameNorm[i], 64);
  }

  int mapped = 0;
  int mismatches = 0;
  for (int s = 0; s < 95; s++) {
    const char *stdName = g_muscleNames[s];
    char stdNorm[64] = {};
    NormalizeMuscle(stdName, stdNorm, 64);

    int found = -1;
    for (int g = 0; g < maxRead; g++) {
      if (gameNorm[g][0] && strcmp(stdNorm, gameNorm[g]) == 0) {
        found = g;
        break;
      }
    }
    if (found >= 0) {
      if (g_stdToGameMap[s] != found) {
        Log("[MUSCLE-MAP] Remap std[%d]=\"%s\" : hardcoded=%d -> dynamic=%d",
            s, stdName, g_stdToGameMap[s], found);
        mismatches++;
      }
      g_stdToGameMap[s] = found;
      mapped++;
    } else {
      Log("[MUSCLE-MAP] WARN: std[%d]=\"%s\" (norm=\"%s\") NOT FOUND! keeping=%d",
          s, stdName, stdNorm, g_stdToGameMap[s]);
    }
  }

  g_dynamicMapReady = true;
  Log("[MUSCLE-MAP] Dynamic map built: %d/95 matched, %d remapped vs hardcoded",
      mapped, mismatches);
  if (mismatches == 0) {
    Log("[MUSCLE-MAP] Hardcoded mapping matches dynamic — no changes needed");
  }
}

static volatile bool g_trojanReentrant = false;
static void *g_gameNativePtr =
    nullptr; 

static void ResetFaceCache() {
  g_faceBonesCaptured = false;
  g_faceBoneRefs = nullptr; 
  g_mouthShapesResolved = false;
  g_extraMorphsResolved = false;
  g_bigListCaptured = false;
  g_hashCorrelationDone = false;
  g_capturedLen = 0;
  g_smcResetRequested = true;

  
  
  for (int i = 0; i < NUM_MOUTH_SHAPES; i++) {
    g_mouthShapes[i].resolved = false;
  }
  for (int i = 0; i < NUM_EXTRA_MORPHS; i++) {
    for (int t = 0; t < g_extraMorphs[i].targetCount; t++) {
      g_extraMorphs[i].targets[t].resolved = false;
    }
  }
  Log("[RESET] Face cache cleared (including per-target resolved flags).");
}



static void CleanupPoseHandler() {
  if (g_poseHandleGC != 0) {
    
    void *managedObj = il2cpp_gchandle_get_target(g_poseHandleGC);
    if (managedObj && g_humanPoseHandler_Dispose) {
      __try {
        void *exc = nullptr;
        il2cpp_runtime_invoke(g_humanPoseHandler_Dispose, managedObj, nullptr,
                              &exc);
        Log("[CLEANUP] HumanPoseHandler.Dispose() called");
      } __except (1) {
        Log("[CLEANUP] Dispose failed (SEH)");
      }
    }
    il2cpp_gchandle_free(g_poseHandleGC);
    Log("[CLEANUP] Released HumanPoseHandler GC handle %u", g_poseHandleGC);
    g_poseHandleGC = 0;
  }
  if (g_musclesArrayGC != 0) {
    il2cpp_gchandle_free(g_musclesArrayGC);
    g_musclesArrayGC = 0;
  }
  g_cachedMPtr = nullptr;
  g_musclesArray = nullptr;
  g_gameNativePtr = nullptr;
  s_poseReady = false;
  
  if (g_trojanHookTarget) {
    MH_DisableHook(g_trojanHookTarget);
    Log("[CLEANUP] Trojan hook disabled");
  }
}

static void __cdecl Hooked_GetInternalAvatarPose(void *nativePtr, void *array,
                                                 int count) {
  
  if (orig_GetInternalAvatarPose) {
    orig_GetInternalAvatarPose(nativePtr, array, count);
  }

  
  if (!g_trojanActive || !g_mmdHasMuscles)
    return;
  if (g_trojanReentrant)
    return;

  
  if (g_cachedMPtr && nativePtr == g_cachedMPtr)
    return;

  
  if (!g_gameNativePtr) {
    g_gameNativePtr = nativePtr;
    Log("[TROJAN] Detected game nativePtr: %p (our m_Ptr: %p, count=%d)",
        nativePtr, g_cachedMPtr, count);
  }

  
  if (nativePtr != g_gameNativePtr)
    return;

  g_trojanReentrant = true;

  
  
  

  if (g_icall_SetInternalHumanPose && s_poseReady && s_musclePtr &&
      g_cachedMPtr) {
    
    memcpy(s_musclePtr, (void *)g_mmdMuscles, 95 * sizeof(float));

    
    float bodyPos[3] = {s_cachedPose.bodyPosX, s_cachedPose.bodyPosY,
                        s_cachedPose.bodyPosZ};
    float bodyRot[4] = {s_cachedPose.bodyRotX, s_cachedPose.bodyRotY,
                        s_cachedPose.bodyRotZ, s_cachedPose.bodyRotW};

    __try {
      
      
      g_icall_SetInternalHumanPose(g_cachedMPtr, bodyPos, bodyRot,
                                   s_cachedPose.muscles);

      
      
      
      if (orig_GetInternalAvatarPose) {
        orig_GetInternalAvatarPose(g_cachedMPtr, array, count);
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      static bool s_errLogged = false;
      if (!s_errLogged) {
        Log("[TROJAN] icall crashed: 0x%08X", GetExceptionCode());
        s_errLogged = true;
      }
    }

    static int s_logCount = 0;
    s_logCount++;

    
    float *avatarData = (float *)array;
    if (s_logCount == 1) {
      Log("[TROJAN-DUMP] Avatar pose buffer: array=%p count=%d (%.0f quats)",
          array, count, count / 4.0f);
      
      for (int i = 0; i < count && i < 80; i += 4) {
        Log("[TROJAN-DUMP] [%d-%d]: %.4f %.4f %.4f %.4f", i, i + 3,
            avatarData[i], avatarData[i + 1], avatarData[i + 2],
            avatarData[i + 3]);
      }
      
      for (int i = 80; i < count && i < 160; i += 4) {
        Log("[TROJAN-DUMP] [%d-%d]: %.4f %.4f %.4f %.4f", i, i + 3,
            avatarData[i], avatarData[i + 1], avatarData[i + 2],
            avatarData[i + 3]);
      }
      
      if (count > 160) {
        Log("[TROJAN-DUMP] ... total %d floats, showing range 160-%d:", count,
            count - 1);
        for (int i = 160; i < count; i += 4) {
          Log("[TROJAN-DUMP] [%d-%d]: %.4f %.4f %.4f %.4f", i, i + 3,
              avatarData[i], avatarData[i + 1], avatarData[i + 2],
              avatarData[i + 3]);
        }
      }
    }

    
    
    
    if (g_mmdHasArmBones && g_muscleAnim && g_muscleAnim->hasArmBones) {
      
      
      
      
      
      
      

      static const int armHBB[ARM_BONE_COUNT] = {13, 15, 17, 14, 16, 18};

      for (int i = 0; i < ARM_BONE_COUNT; i++) {
        float *mmdCur = (float *)&g_mmdArmBoneRots[i * 4];
        float *mmdRest = &g_muscleAnim->armRestRots[i * 4];

        
        
        
        
        int boneIdx = armHBB[i];
        int offset = boneIdx * 4;

        if (offset + 3 < count) {
          
          float invMR[4] = {-mmdRest[0], -mmdRest[1], -mmdRest[2], mmdRest[3]};
          float dx = invMR[3] * mmdCur[0] + invMR[0] * mmdCur[3] +
                     invMR[1] * mmdCur[2] - invMR[2] * mmdCur[1];
          float dy = invMR[3] * mmdCur[1] - invMR[0] * mmdCur[2] +
                     invMR[1] * mmdCur[3] + invMR[2] * mmdCur[0];
          float dz = invMR[3] * mmdCur[2] + invMR[0] * mmdCur[1] -
                     invMR[1] * mmdCur[0] + invMR[2] * mmdCur[3];
          float dw = invMR[3] * mmdCur[3] - invMR[0] * mmdCur[0] -
                     invMR[1] * mmdCur[1] - invMR[2] * mmdCur[2];

          
          float *gr =
              &avatarData[offset]; 
          
          static float s_origArm[ARM_BONE_COUNT * 4] = {};
          static bool s_origSaved = false;
          if (!s_origSaved && s_logCount == 1) {
            for (int j = 0; j < ARM_BONE_COUNT; j++) {
              int off = armHBB[j] * 4;
              if (off + 3 < count)
                memcpy(&s_origArm[j * 4], &avatarData[off], 16);
            }
            s_origSaved = true;
          }

          
          
          float ox = gr[0], oy = gr[1], oz = gr[2], ow = gr[3];
          gr[0] = ow * dx + ox * dw + oy * dz - oz * dy;
          gr[1] = ow * dy - ox * dz + oy * dw + oz * dx;
          gr[2] = ow * dz + ox * dy - oy * dx + oz * dw;
          gr[3] = ow * dw - ox * dx - oy * dy - oz * dz;
          float len = sqrtf(gr[0] * gr[0] + gr[1] * gr[1] + gr[2] * gr[2] +
                            gr[3] * gr[3]);
          if (len > 0.001f) {
            gr[0] /= len;
            gr[1] /= len;
            gr[2] /= len;
            gr[3] /= len;
          }
        }
      }
    }


    if (s_logCount <= 3 || (s_logCount % 300 == 0 && s_logCount <= 3000)) {
      Log("[TROJAN] #%d: direct icall applied! m[0]=%.3f m[42]=%.3f "
          "armOverride=%s",
          s_logCount, (float)g_mmdMuscles[0], (float)g_mmdMuscles[42],
          g_mmdHasArmBones ? "YES" : "no");
    }
  }

  g_trojanReentrant = false;
}




#define WM_MMD_APPLY_POSE (WM_USER + 1)

static WNDPROC g_origWndProc = nullptr;
static volatile bool g_mmdPendingApply = false;
static volatile bool g_mmdSetMode = false;


typedef void *(*InvokerFn)(void *methodPtr, void *method, void *obj,
                           void **params, void *retval);
static InvokerFn orig_Invoker = nullptr;
static void *__cdecl Hooked_Invoker(void *methodPtr, void *method, void *obj,
                                    void **params, void *retval) {
  return orig_Invoker(methodPtr, method, obj, params, retval);
}









typedef void(__cdecl *fn_SetHP_icall)(void *handler, void *bodyPos,
                                      void *bodyRot, void *muscles,
                                      void *methodInfo);
static fn_SetHP_icall s_directSetHP = nullptr;


static void InitMmdPoseOnMainThread() {
  if (s_poseReady)
    return; 
  if (!g_muscleAnim || !g_muscleAnim->loaded)
    return;
  if (!g_poseHandleGC)
    return;
  if (!g_slotAddr || !g_slotSetFn)
    return;

  void *managedObj = il2cpp_gchandle_get_target(g_poseHandleGC);
  if (!managedObj)
    return;

  
  void *getArgs[] = {&s_cachedPose};
  void *getExc = nullptr;
  il2cpp_runtime_invoke(g_humanPoseHandler_GetHumanPose, managedObj, getArgs,
                        &getExc);
  if (getExc || !s_cachedPose.muscles) {
    Log("[MMD-INIT] GetHumanPose failed: exc=%p muscles=%p", getExc,
        s_cachedPose.muscles);
    return;
  }

  s_musclePtr = GetArrayData(s_cachedPose.muscles);
  if (!s_musclePtr)
    return;

  
  uint64_t arrayLen = *(uint64_t *)((char *)s_cachedPose.muscles + 24);
  s_actualMuscleCount = (int)arrayLen;
  if (s_actualMuscleCount > 128)
    s_actualMuscleCount = 128;
  Log("[MMD-INIT] Muscles array length = %llu (expected 95, got %d)", arrayLen,
      s_actualMuscleCount);

  
  if (!g_dynamicMapReady)
    BuildDynamicMuscleMap();

  
  if (*g_slotAddr) {
    g_slotOrigGet = *g_slotAddr;
    Log("[SLOT] Captured Get fn: %p", g_slotOrigGet);
  }

  
  uint32_t h = il2cpp_gchandle_new(s_cachedPose.muscles, true);

  
  s_directSetHP = (fn_SetHP_icall)g_slotSetFn;

  Log("[MMD-INIT] Ready! m_Ptr=%p muscles=%p setFn=%p", g_cachedMPtr,
      s_cachedPose.muscles, s_directSetHP);
  Log("[MMD-INIT] Rest: pos(%.3f,%.3f,%.3f) rot(%.3f,%.3f,%.3f,%.3f)",
      s_cachedPose.bodyPosX, s_cachedPose.bodyPosY, s_cachedPose.bodyPosZ,
      s_cachedPose.bodyRotX, s_cachedPose.bodyRotY, s_cachedPose.bodyRotZ,
      s_cachedPose.bodyRotW);

  
  memcpy(s_savedIdleMuscles, s_musclePtr, s_actualMuscleCount * sizeof(float));
  Log("[MMD-INIT] Saved %d idle muscles", s_actualMuscleCount);
  s_savedIdleBodyPos[0] = s_cachedPose.bodyPosX;
  s_savedIdleBodyPos[1] = s_cachedPose.bodyPosY;
  s_savedIdleBodyPos[2] = s_cachedPose.bodyPosZ;
  s_savedIdleBodyRot[0] = s_cachedPose.bodyRotX;
  s_savedIdleBodyRot[1] = s_cachedPose.bodyRotY;
  s_savedIdleBodyRot[2] = s_cachedPose.bodyRotZ;
  s_savedIdleBodyRot[3] = s_cachedPose.bodyRotW;
  Log("[MMD-INIT] Saved idle muscles: arm39=%.3f arm48=%.3f fore42=%.3f "
      "fore51=%.3f",
      s_savedIdleMuscles[39], s_savedIdleMuscles[48], s_savedIdleMuscles[42],
      s_savedIdleMuscles[51]);
  
  Log("[REST-MUSCLES] Spine[0-8]:  %.3f %.3f %.3f | %.3f %.3f %.3f | %.3f %.3f "
      "%.3f",
      s_musclePtr[0], s_musclePtr[1], s_musclePtr[2], s_musclePtr[3],
      s_musclePtr[4], s_musclePtr[5], s_musclePtr[6], s_musclePtr[7],
      s_musclePtr[8]);
  Log("[REST-MUSCLES] NeckHead[9-14]: %.3f %.3f %.3f | %.3f %.3f %.3f",
      s_musclePtr[9], s_musclePtr[10], s_musclePtr[11], s_musclePtr[12],
      s_musclePtr[13], s_musclePtr[14]);
  Log("[REST-MUSCLES] EyeJaw[15-20]: %.3f %.3f %.3f %.3f | %.3f %.3f",
      s_musclePtr[15], s_musclePtr[16], s_musclePtr[17], s_musclePtr[18],
      s_musclePtr[19], s_musclePtr[20]);
  Log("[REST-MUSCLES] LLeg[21-28]: %.3f %.3f %.3f | %.3f %.3f | %.3f %.3f | "
      "%.3f",
      s_musclePtr[21], s_musclePtr[22], s_musclePtr[23], s_musclePtr[24],
      s_musclePtr[25], s_musclePtr[26], s_musclePtr[27], s_musclePtr[28]);
  Log("[REST-MUSCLES] RLeg[29-36]: %.3f %.3f %.3f | %.3f %.3f | %.3f %.3f | "
      "%.3f",
      s_musclePtr[29], s_musclePtr[30], s_musclePtr[31], s_musclePtr[32],
      s_musclePtr[33], s_musclePtr[34], s_musclePtr[35], s_musclePtr[36]);
  Log("[REST-MUSCLES] LArm[37-45]: %.3f %.3f | %.3f %.3f %.3f | %.3f %.3f | "
      "%.3f %.3f",
      s_musclePtr[37], s_musclePtr[38], s_musclePtr[39], s_musclePtr[40],
      s_musclePtr[41], s_musclePtr[42], s_musclePtr[43], s_musclePtr[44],
      s_musclePtr[45]);
  Log("[REST-MUSCLES] RArm[46-54]: %.3f %.3f | %.3f %.3f %.3f | %.3f %.3f | "
      "%.3f %.3f",
      s_musclePtr[46], s_musclePtr[47], s_musclePtr[48], s_musclePtr[49],
      s_musclePtr[50], s_musclePtr[51], s_musclePtr[52], s_musclePtr[53],
      s_musclePtr[54]);
  Log("[REST-MUSCLES] LFinger[55-74]: %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f "
      "%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f",
      s_musclePtr[55], s_musclePtr[56], s_musclePtr[57], s_musclePtr[58],
      s_musclePtr[59], s_musclePtr[60], s_musclePtr[61], s_musclePtr[62],
      s_musclePtr[63], s_musclePtr[64], s_musclePtr[65], s_musclePtr[66],
      s_musclePtr[67], s_musclePtr[68], s_musclePtr[69], s_musclePtr[70],
      s_musclePtr[71], s_musclePtr[72], s_musclePtr[73], s_musclePtr[74]);
  Log("[REST-MUSCLES] RFinger[75-94]: %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f "
      "%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f",
      s_musclePtr[75], s_musclePtr[76], s_musclePtr[77], s_musclePtr[78],
      s_musclePtr[79], s_musclePtr[80], s_musclePtr[81], s_musclePtr[82],
      s_musclePtr[83], s_musclePtr[84], s_musclePtr[85], s_musclePtr[86],
      s_musclePtr[87], s_musclePtr[88], s_musclePtr[89], s_musclePtr[90],
      s_musclePtr[91], s_musclePtr[92], s_musclePtr[93], s_musclePtr[94]);

  
  memcpy(s_musclePtr, (void *)g_mmdMuscles, 95 * sizeof(float));
  *g_slotAddr = g_slotSetFn;
  void *setArgs[] = {&s_cachedPose};
  void *setExc = nullptr;
  il2cpp_runtime_invoke(g_humanPoseHandler_GetHumanPose, managedObj, setArgs,
                        &setExc);
  *g_slotAddr = g_slotOrigGet;
  Log("[MMD-INIT] Verification Set: exc=%s", setExc ? "ERR" : "OK");

  s_poseReady = true;
}


static void ApplyMmdPoseDirect() {
  if (!s_poseReady || !s_directSetHP || !g_cachedMPtr)
    return;

  
  SafeSetAnimatorEnabled(false);

  
  memcpy(s_musclePtr, (void *)g_mmdMuscles, 95 * sizeof(float));

  
  float bodyPos[3] = {s_cachedPose.bodyPosX, s_cachedPose.bodyPosY,
                      s_cachedPose.bodyPosZ};
  float bodyRot[4] = {s_cachedPose.bodyRotX, s_cachedPose.bodyRotY,
                      s_cachedPose.bodyRotZ, s_cachedPose.bodyRotW};

  
  
  __try {
    s_directSetHP(g_cachedMPtr, bodyPos, bodyRot, s_cachedPose.muscles,
                  nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    static bool s_errLogged = false;
    if (!s_errLogged) {
      Log("[MMD-DIRECT] icall crashed: 0x%08X —falling back to slot-patch",
          GetExceptionCode());
      s_errLogged = true;
    }
    
    if (g_gameHwnd && !g_mmdPendingApply) {
      g_mmdPendingApply = true;
      PostMessageW(g_gameHwnd, WM_MMD_APPLY_POSE, 0, 0);
    }
    return;
  }

  static int s_frameCount = 0;
  s_frameCount++;
  if (s_frameCount <= 3 || (s_frameCount % 300 == 0 && s_frameCount <= 3000)) {
    Log("[MMD-DIRECT] #%d: m[0]=%.3f m[21]=%.3f m[42]=%.3f", s_frameCount,
        (float)g_mmdMuscles[0], (float)g_mmdMuscles[21],
        (float)g_mmdMuscles[42]);
  }
}







#define MAX_HUMAN_BONES 55
struct CachedBoneState {
  void *transform; 
  Quat rotation;   
  bool valid;
};
static CachedBoneState g_cachedBones[MAX_HUMAN_BONES] = {};
static Vec3 g_cachedHipsPos = {};
static void *g_hipsTransform = nullptr;
static volatile bool g_bonesReady = false; 
static volatile int g_boneGeneration = 0;  


static volatile int g_debugMuscleIdx = 0;      
static volatile float g_debugMuscleVal = 0.0f; 
static volatile bool g_debugMode = false;
static volatile bool g_debugDirty = true; 
static volatile int g_debugMaxIdx =
    95; 









static int StandardToGame(int stdIdx) {
  if (stdIdx < 0 || stdIdx > 94)
    return -1;
  return g_stdToGameMap[stdIdx];
}

static void ApplyMmdPoseOnMainThread() {
  if (!s_poseReady || !s_musclePtr)
    return;
  if (!g_poseHandleGC || !g_slotAddr || !g_slotOrigGet)
    return;

  void *managedObj = il2cpp_gchandle_get_target(g_poseHandleGC);
  if (!managedObj)
    return;

  SafeSetAnimatorEnabled(false);

  
  if (g_debugMode) {
    int mc = s_actualMuscleCount;
    for (int i = 0; i < mc; i++) {
      s_musclePtr[i] = s_savedIdleMuscles[i];
    }
    int idx = g_debugMuscleIdx;
    if (idx >= 0 && idx < mc) {
      s_musclePtr[idx] = s_savedIdleMuscles[idx] + g_debugMuscleVal;
    }
    s_cachedPose.bodyPosX = s_savedIdleBodyPos[0];
    s_cachedPose.bodyPosY = s_savedIdleBodyPos[1];
    s_cachedPose.bodyPosZ = s_savedIdleBodyPos[2];
    s_cachedPose.bodyRotX = s_savedIdleBodyRot[0];
    s_cachedPose.bodyRotY = s_savedIdleBodyRot[1];
    s_cachedPose.bodyRotZ = s_savedIdleBodyRot[2];
    s_cachedPose.bodyRotW = s_savedIdleBodyRot[3];

    *g_slotAddr = g_slotSetFn;
    void *setArgs[] = {&s_cachedPose};
    void *setExc = nullptr;
    il2cpp_runtime_invoke(g_humanPoseHandler_GetHumanPose, managedObj, setArgs,
                          &setExc);
    *g_slotAddr = g_slotOrigGet;

    if (g_debugDirty) {
      const char *name =
          (idx >= 0 && idx < 95) ? g_muscleNames[idx] : "(EXTRA)";
      Log("[DEBUG] muscle[%d/%d] = idle(%.3f) + %.1f = %.3f  \"%s\"", idx,
          s_actualMuscleCount, s_savedIdleMuscles[idx], (float)g_debugMuscleVal,
          s_musclePtr[idx], name);
      g_debugDirty = false;
    }
    return;
  }

  
  static float s_mmdFirstMuscles[95] = {};
  static float s_mmdFirstBodyPos[3] = {};
  static float s_mmdFirstBodyRot[4] = {};
  static bool s_firstFrame = true;

  if (s_firstFrame) {
    memcpy(s_mmdFirstMuscles, (void *)g_mmdMuscles, 95 * sizeof(float));
    s_mmdFirstBodyPos[0] = g_mmdBodyPos[0];
    s_mmdFirstBodyPos[1] = g_mmdBodyPos[1];
    s_mmdFirstBodyPos[2] = g_mmdBodyPos[2];
    s_mmdFirstBodyRot[0] = g_mmdBodyRot[0];
    s_mmdFirstBodyRot[1] = g_mmdBodyRot[1];
    s_mmdFirstBodyRot[2] = g_mmdBodyRot[2];
    s_mmdFirstBodyRot[3] = g_mmdBodyRot[3];
    Log("[REMAP] 95—01 remapping active. First frame captured.");
    Log("[REMAP] MMD arm39→game%d arm48→game%d "
        "fore42→game%d",
        StandardToGame(39), StandardToGame(48), StandardToGame(42));
    s_firstFrame = false;
  }

  
  
  int mc = s_actualMuscleCount;
  for (int i = 0; i < mc; i++) {
    s_musclePtr[i] = 0.0f;
  }
  
  
  {
    bool mapped[128] = {};
    for (int s = 0; s < 95; s++) {
      int g = StandardToGame(s);
      if (g >= 0 && g < mc)
        mapped[g] = true;
    }
    for (int i = 0; i < mc; i++) {
      if (!mapped[i])
        s_musclePtr[i] = s_savedIdleMuscles[i];
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  static const float SHOULDER_DU_OFFSET = 0.0f;
  
  static const float ARM_DU_OFFSET = -0.10f;
  
  
  static const float ARM_FB_OFFSET = 0.12f;
  
  static const float FOREARM_STRETCH_OFFSET = -0.08f;
  
  static const float NECK_NOD_OFFSET = 0.05f;

  for (int stdIdx = 0; stdIdx < 95; stdIdx++) {
    int gameIdx = StandardToGame(stdIdx);
    if (gameIdx < 0 || gameIdx >= mc)
      continue;

    float mmdCur = ((volatile float *)g_mmdMuscles)[stdIdx];

    
    
    
    
    
    if (stdIdx >= 15 && stdIdx <= 20)
      continue;

    
    switch (stdIdx) {
    case 9:
      mmdCur += NECK_NOD_OFFSET;
      break; 
    case 39:
      mmdCur += ARM_DU_OFFSET;
      break; 
    case 48:
      mmdCur += ARM_DU_OFFSET;
      break; 
    case 40:
      mmdCur += ARM_FB_OFFSET;
      break; 
    case 49:
      mmdCur += ARM_FB_OFFSET;
      break; 
    case 42:
      mmdCur += FOREARM_STRETCH_OFFSET;
      break; 
    case 51:
      mmdCur += FOREARM_STRETCH_OFFSET;
      break; 
    }

    s_musclePtr[gameIdx] = mmdCur;
  }

  
  
  
  
  
  s_cachedPose.bodyPosX =
      g_restBodyPos[0] + (g_mmdBodyPos[0] - s_mmdFirstBodyPos[0]);
  s_cachedPose.bodyPosY =
      g_restBodyPos[1] + (g_mmdBodyPos[1] - s_mmdFirstBodyPos[1]);
  s_cachedPose.bodyPosZ =
      g_restBodyPos[2] + (g_mmdBodyPos[2] - s_mmdFirstBodyPos[2]);

  
  s_cachedPose.bodyRotX = g_mmdBodyRot[0];
  s_cachedPose.bodyRotY = g_mmdBodyRot[1];
  s_cachedPose.bodyRotZ = g_mmdBodyRot[2];
  s_cachedPose.bodyRotW = g_mmdBodyRot[3];
  float qlen = sqrtf(s_cachedPose.bodyRotX * s_cachedPose.bodyRotX +
                     s_cachedPose.bodyRotY * s_cachedPose.bodyRotY +
                     s_cachedPose.bodyRotZ * s_cachedPose.bodyRotZ +
                     s_cachedPose.bodyRotW * s_cachedPose.bodyRotW);
  if (qlen > 0.001f) {
    s_cachedPose.bodyRotX /= qlen;
    s_cachedPose.bodyRotY /= qlen;
    s_cachedPose.bodyRotZ /= qlen;
    s_cachedPose.bodyRotW /= qlen;
  }

  
  *g_slotAddr = g_slotSetFn;
  void *setArgs[] = {&s_cachedPose};
  void *setExc = nullptr;
  il2cpp_runtime_invoke(g_humanPoseHandler_GetHumanPose, managedObj, setArgs,
                        &setExc);
  *g_slotAddr = g_slotOrigGet;

  
  
  
  
  if (g_mmdHasArmBones && g_muscleAnim && g_muscleAnim->hasArmBones &&
      g_cachedAnimator) {

    
    if (!s_ikDisabled) {
      s_ikDisabled = true;

      void *animatorGO = nullptr;
      __try {
        animatorGO = Invoke(g_component_get_gameObject, g_cachedAnimator);
      } __except (1) {
      }

      if (animatorGO) {
        void *rootTransform = SafeGetComponentTransform(g_cachedAnimator);
        if (rootTransform) {
          
          struct WalkEntry {
            void *t;
            int d;
          };
          WalkEntry stack[64];
          int top = 0;
          stack[top++] = {rootTransform, 0};

          while (top > 0) {
            WalkEntry e = stack[--top];

            
            void *go = nullptr;
            __try {
              go = Invoke(g_component_get_gameObject, e.t);
            } __except (1) {
            }

            if (go) {
              
              void *componentClass = nullptr;
              void *componentType = nullptr;
              {
                void *domain = il2cpp_domain_get();
                size_t ac;
                void **asms = il2cpp_domain_get_assemblies(domain, &ac);
                componentClass =
                    FindClass("UnityEngine", "Component", asms, ac);
              }
              if (componentClass)
                componentType = il2cpp_class_get_type(componentClass);

              if (componentType) {
                void *getCompMethod =
                    FindMethod(il2cpp_object_get_class(go), "GetComponents", 1);
                void *typeObj = nullptr;
                __try {
                  typeObj = il2cpp_type_get_object(componentType);
                } __except (1) {
                }

                if (getCompMethod && typeObj) {
                  void *exc = nullptr;
                  void *args[] = {typeObj};
                  void *arr = nullptr;
                  __try {
                    arr = il2cpp_runtime_invoke(getCompMethod, go, args, &exc);
                  } __except (1) {
                  }

                  if (arr && !exc) {
                    int cnt = *(int *)((char *)arr + 24);
                    void **data = (void **)((char *)arr + 32);

                    for (int i = 0; i < cnt; i++) {
                      if (!data[i])
                        continue;
                      void *cls = il2cpp_object_get_class(data[i]);
                      const char *clsName =
                          cls ? il2cpp_class_get_name(cls) : "";

                      if (strcmp(clsName, "BipedIK") == 0 &&
                          s_bipedIKCount < MAX_IK) {
                        
                        
                        char ikGoName[64] = "?";
                        __try {
                          if (g_object_get_name) {
                            void *ns = Invoke(g_object_get_name, go);
                            if (ns) ReadStrUtf8(ns, ikGoName, sizeof(ikGoName));
                          }
                        } __except (1) {}
                        s_bipedIK[s_bipedIKCount++] = data[i];
                        Log("[IK-DISABLE] Found BipedIK #%d: %p GO='%s'",
                            s_bipedIKCount, data[i], ikGoName);
                      }
                      if (strcmp(clsName, "GrounderBipedIK") == 0 &&
                          s_grounderIKCount < MAX_IK) {
                        char gkGoName[64] = "?";
                        __try {
                          if (g_object_get_name) {
                            void *ns = Invoke(g_object_get_name, go);
                            if (ns) ReadStrUtf8(ns, gkGoName, sizeof(gkGoName));
                          }
                        } __except (1) {}
                        s_grounderIK[s_grounderIKCount++] = data[i];
                        Log("[IK-DISABLE] Found GrounderBipedIK #%d: %p GO='%s'",
                            s_grounderIKCount, data[i], gkGoName);
                      }
                      if (strcmp(clsName, "TransformFollowDamper") == 0 &&
                          s_followDamperCount < 4) {
                        char fdGoName[64] = "?";
                        __try {
                          if (g_object_get_name) {
                            void *ns = Invoke(g_object_get_name, go);
                            if (ns) ReadStrUtf8(ns, fdGoName, sizeof(fdGoName));
                          }
                        } __except (1) {}
                        s_followDamper[s_followDamperCount++] = data[i];
                        Log("[IK-DISABLE] Found TransformFollowDamper #%d: %p GO='%s'",
                            s_followDamperCount, data[i], fdGoName);
                      }
                      if (strcmp(clsName, "AnimatorMono") == 0) {
                        char amGoName[64] = "?";
                        __try {
                          if (g_object_get_name) {
                            void *ns = Invoke(g_object_get_name, go);
                            if (ns) ReadStrUtf8(ns, amGoName, sizeof(amGoName));
                          }
                        } __except (1) {}
                        s_animatorMono = data[i];
                        Log("[IK-DISABLE] Found AnimatorMono: %p GO='%s'",
                            s_animatorMono, amGoName);
                      }
                      
                      if (strcmp(clsName, "BeyondBoneCloth") == 0 &&
                          s_bbcCount < MAX_BBC) {
                        int idx = s_bbcCount;
                        s_bbcInstances[s_bbcCount++] = data[i];
                        Log("[BBC] Found BeyondBoneCloth #%d: %p",
                            s_bbcCount, data[i]);
                        
                        __try {
                          if (g_component_get_gameObject && g_object_get_name) {
                            void *go = Invoke(g_component_get_gameObject, data[i]);
                            if (go) {
                              void *nameStr = Invoke(g_object_get_name, go);
                              char goName[64] = "";
                              if (nameStr) ReadStrUtf8(nameStr, goName, sizeof(goName));
                              Log("[BBC] #%d GO name='%s'", idx + 1, goName);
                              if (_stricmp(goName, "MC_skirt") == 0 ||
                                  _stricmp(goName, "MC_Skirt") == 0 ||
                                  strstr(goName, "skirt") || strstr(goName, "Skirt")) {
                                s_skirtBBCIndex = idx;
                                Log("[BBC] MC_Skirt identified at index %d (by name '%s')", idx, goName);
                              }
                            } else {
                              Log("[BBC] #%d get_gameObject returned null", idx + 1);
                            }
                          } else {
                            Log("[BBC] #%d name resolve skipped: getGO=%p getName=%p",
                                idx + 1, g_component_get_gameObject, g_object_get_name);
                          }
                        } __except (1) {
                          Log("[BBC] #%d name resolve exception", idx + 1);
                        }
                      }
                    }
                  }
                }
              }
            }

            
            if (e.d < 2) {
              __try {
                void *ccResult = Invoke(g_transform_get_childCount, e.t);
                int cc = ccResult ? *(int *)((char *)ccResult + 16) : 0;
                for (int c = 0; c < cc && top < 64; c++) {
                  void *cp[] = {&c};
                  void *child = Invoke(g_transform_GetChild, e.t, cp);
                  if (child)
                    stack[top++] = {child, e.d + 1};
                }
              } __except (EXCEPTION_EXECUTE_HANDLER) {
              }
            }
          }

          
          
          for (int bi = 0; bi < s_bipedIKCount; bi++) {
            if (s_bipedIK[bi] && g_animator_set_enabled) {
              __try {
                int falseVal = 0;
                void *params[] = {&falseVal};
                Invoke(g_animator_set_enabled, s_bipedIK[bi], params);
                Log("[IK-DISABLE] BipedIK #%d DISABLED!", bi + 1);
              } __except (EXCEPTION_EXECUTE_HANDLER) {
                Log("[IK-DISABLE] Failed to disable BipedIK #%d: 0x%08X",
                    bi + 1, GetExceptionCode());
              }
            }
          }

          for (int gi = 0; gi < s_grounderIKCount; gi++) {
            if (s_grounderIK[gi] && g_animator_set_enabled) {
              __try {
                int falseVal = 0;
                void *params[] = {&falseVal};
                Invoke(g_animator_set_enabled, s_grounderIK[gi], params);
                Log("[IK-DISABLE] GrounderBipedIK #%d DISABLED!", gi + 1);
              } __except (EXCEPTION_EXECUTE_HANDLER) {
                Log("[IK-DISABLE] Failed to disable GrounderBipedIK #%d", gi + 1);
              }
            }
          }

          
          
          
          
          
          for (int fd = 0; fd < s_followDamperCount; fd++) {
            Log("[IK-DISABLE] TransformFollowDamper #%d KEPT ENABLED (decoration)", fd + 1);
          }

          
          if (s_animatorMono && g_animator_set_enabled) {
            __try {
              int falseVal = 0;
              void *params[] = {&falseVal};
              Invoke(g_animator_set_enabled, s_animatorMono, params);
              Log("[IK-DISABLE] AnimatorMono DISABLED!");
            } __except (EXCEPTION_EXECUTE_HANDLER) {
            }
          }

          if (s_bipedIKCount == 0)
            Log("[IK-DISABLE] WARNING: BipedIK not found!");

          
          
          if (s_bbcCount > 0 && !s_bbcMethodsResolved) {
            s_bbcMethodsResolved = true;
            void *bbcCls = il2cpp_object_get_class(s_bbcInstances[0]);
            if (bbcCls) {
              void *miter2 = nullptr;
              void *m2 = nullptr;
              while ((m2 = il2cpp_class_get_methods(bbcCls, &miter2))) {
                const char *mn2 = il2cpp_method_get_name(m2);
                if (!mn2) continue;
                if (strcmp(mn2, "ResetCloth") == 0)
                  s_bbc_ResetCloth = m2;
                if (strcmp(mn2, "SetAnimationPoseRatio") == 0)
                  s_bbc_SetAnimPoseRatio = m2;
                if (strcmp(mn2, "SetClothSimulateWeight") == 0)
                  s_bbc_SetSimWeight = m2;
                if (strcmp(mn2, "BuildAndRun") == 0)
                  s_bbc_BuildAndRun = m2;
                if (strcmp(mn2, "SetSkipWriting") == 0)
                  s_bbc_SetSkipWriting = m2;
                if (strcmp(mn2, "SetTimeScale") == 0)
                  s_bbc_SetTimeScale = m2;
              }
              Log("[BBC] Methods: Reset=%p Ratio=%p Weight=%p Build=%p Skip=%p",
                  s_bbc_ResetCloth, s_bbc_SetAnimPoseRatio, s_bbc_SetSimWeight,
                  s_bbc_BuildAndRun, s_bbc_SetSkipWriting);
            }
          }
          
          
          
          
          
          
          
          if (s_bbcCount > 0 && g_animator_set_enabled) {
            int falseVal = 0, trueVal = 1;
            float one = 1.0f;
            for (int bi = 0; bi < s_bbcCount; bi++) {
              if (!s_bbcInstances[bi]) continue;
              __try {
                
                void *offArgs[] = {&falseVal};
                Invoke(g_animator_set_enabled, s_bbcInstances[bi], offArgs);
                void *onArgs[] = {&trueVal};
                Invoke(g_animator_set_enabled, s_bbcInstances[bi], onArgs);
                
                if (s_bbc_SetSkipWriting) {
                  void *skipArgs[] = {&falseVal};
                  void *exc = nullptr;
                  il2cpp_runtime_invoke(s_bbc_SetSkipWriting,
                                       s_bbcInstances[bi], skipArgs, &exc);
                }
                
                if (s_bbc_SetSimWeight) {
                  void *swArgs[] = {&one};
                  void *exc = nullptr;
                  il2cpp_runtime_invoke(s_bbc_SetSimWeight,
                                       s_bbcInstances[bi], swArgs, &exc);
                }
                
                if (s_bbc_BuildAndRun) {
                  void *exc = nullptr;
                  il2cpp_runtime_invoke(s_bbc_BuildAndRun,
                                       s_bbcInstances[bi], nullptr, &exc);
                }
              } __except (1) {}
            }
            Log("[BBC] Full restart on %d instances (colliders scaled + rebuild)",
                s_bbcCount);

            
            
            
            s_skirtScaleResolved = false;
            s_skirtDirty = true; 
            ApplySkirtColliderScale();
          }
        }
      }
    }
    
  
  
  if (g_mmdHasFingerBones && g_muscleAnim && g_muscleAnim->hasFingerBones) {
    
    if (!g_fingerTransformsResolved) {
      g_fingerTransformsResolved = true;
      static const char *fingerNames[FINGER_BONE_COUNT] = {
          "Bip001_L_Finger0",  "Bip001_L_Finger01", "Bip001_L_Finger02",
          "Bip001_L_Finger1",  "Bip001_L_Finger11", "Bip001_L_Finger12",
          "Bip001_L_Finger2",  "Bip001_L_Finger21", "Bip001_L_Finger22",
          "Bip001_L_Finger3",  "Bip001_L_Finger31", "Bip001_L_Finger32",
          "Bip001_L_Finger4",  "Bip001_L_Finger41", "Bip001_L_Finger42",
          "Bip001_R_Finger0",  "Bip001_R_Finger01", "Bip001_R_Finger02",
          "Bip001_R_Finger1",  "Bip001_R_Finger11", "Bip001_R_Finger12",
          "Bip001_R_Finger2",  "Bip001_R_Finger21", "Bip001_R_Finger22",
          "Bip001_R_Finger3",  "Bip001_R_Finger31", "Bip001_R_Finger32",
          "Bip001_R_Finger4",  "Bip001_R_Finger41", "Bip001_R_Finger42",
      };
      void *rootT = SafeGetComponentTransform(g_cachedAnimator);
      int found = 0;
      if (rootT) {
        for (int i = 0; i < FINGER_BONE_COUNT; i++) {
          g_fingerTransforms[i] =
              SafeFindChildRecursive(rootT, fingerNames[i], 15);
          if (g_fingerTransforms[i])
            found++;
        }
      }
      Log("[FINGER-ANIM] Discovered %d/%d finger bones", found,
          FINGER_BONE_COUNT);
    }

    
    if (!g_fingerRestCaptured) {
      g_fingerRestCaptured = true;
      for (int i = 0; i < FINGER_BONE_COUNT; i++) {
        if (!g_fingerTransforms[i]) {
          g_gameFingerRest[i * 4 + 0] = 0;
          g_gameFingerRest[i * 4 + 1] = 0;
          g_gameFingerRest[i * 4 + 2] = 0;
          g_gameFingerRest[i * 4 + 3] = 1;
          continue;
        }
        Quat gr = SafeGetLocalRotation(g_fingerTransforms[i]);
        g_gameFingerRest[i * 4 + 0] = gr.x;
        g_gameFingerRest[i * 4 + 1] = gr.y;
        g_gameFingerRest[i * 4 + 2] = gr.z;
        g_gameFingerRest[i * 4 + 3] = gr.w;
      }
      Log("[FINGER-ANIM] Captured game finger rest rotations");
    }

    
    
    
    
    for (int i = 0; i < FINGER_BONE_COUNT; i++) {
      if (!g_fingerTransforms[i])
        continue;
      float *mmdCur = (float *)&g_mmdFingerBoneRots[i * 4];
      float *mmdRest = &g_muscleAnim->fingerRestRots[i * 4];

      
      float invR[4] = {-mmdRest[0], -mmdRest[1], -mmdRest[2], mmdRest[3]};
      float dx = invR[3] * mmdCur[0] + invR[0] * mmdCur[3] +
                 invR[1] * mmdCur[2] - invR[2] * mmdCur[1];
      float dy = invR[3] * mmdCur[1] - invR[0] * mmdCur[2] +
                 invR[1] * mmdCur[3] + invR[2] * mmdCur[0];
      float dz = invR[3] * mmdCur[2] + invR[0] * mmdCur[1] -
                 invR[1] * mmdCur[0] + invR[2] * mmdCur[3];
      float dw = invR[3] * mmdCur[3] - invR[0] * mmdCur[0] -
                 invR[1] * mmdCur[1] - invR[2] * mmdCur[2];

      
      if (i < 15) {
        dx = -dx; dy = -dy; dz = -dz;
      }

      
      
      float dot = dw; 
      if (dot < 0) { dx = -dx; dy = -dy; dz = -dz; dw = -dw; dot = -dot; }
      float t = 0.75f;
      float sx, sy;
      if (dot > 0.9995f) {
        
        sx = 1.0f - t;
        sy = t;
      } else {
        float theta = acosf(dot);
        float sinT = sinf(theta);
        sx = sinf((1.0f - t) * theta) / sinT;
        sy = sinf(t * theta) / sinT;
      }
      
      float sdx = sy * dx;
      float sdy = sy * dy;
      float sdz = sy * dz;
      float sdw = sx + sy * dw;

      
      float *gr = &g_gameFingerRest[i * 4];
      float rx = gr[3] * sdx + gr[0] * sdw + gr[1] * sdz - gr[2] * sdy;
      float ry = gr[3] * sdy - gr[0] * sdz + gr[1] * sdw + gr[2] * sdx;
      float rz = gr[3] * sdz + gr[0] * sdy - gr[1] * sdx + gr[2] * sdw;
      float rw = gr[3] * sdw - gr[0] * sdx - gr[1] * sdy - gr[2] * sdz;

      
      float len = sqrtf(rx * rx + ry * ry + rz * rz + rw * rw);
      if (len > 0.001f) { rx /= len; ry /= len; rz /= len; rw /= len; }

      SafeSetLocalRotation(g_fingerTransforms[i], {rx, ry, rz, rw});
    }
  }

  
    
  }

  static int s_cnt = 0;
  s_cnt++;
  if (s_cnt <= 3 || (s_cnt % 300 == 0 && s_cnt <= 3000)) {
    int g39 = StandardToGame(39), g48 = StandardToGame(48),
        g42 = StandardToGame(42);
    float dxPos = g_mmdBodyPos[0] - s_mmdFirstBodyPos[0];
    float dyPos = g_mmdBodyPos[1] - s_mmdFirstBodyPos[1];
    float dzPos = g_mmdBodyPos[2] - s_mmdFirstBodyPos[2];
    Log("[MMD] #%d: arm39→g%d=%.2f arm48→g%d=%.2f "
        "fore42→g%d=%.2f bodyD=(%.3f,%.3f,%.3f) exc=%s",
        s_cnt, g39, s_musclePtr[g39], g48, s_musclePtr[g48], g42,
        s_musclePtr[g42], dxPos, dyPos, dzPos, setExc ? "ERR" : "OK");
  }

  
  
  
  if (g_cameraNeedsCapture && g_cameraPlayer.HasData()) {
    g_cameraNeedsCapture = false;
    
    __try {
      void *charTransform = SafeGetComponentTransform(g_cachedAnimator);
      if (charTransform && g_camGetPos) {
        float pos[3] = {};
        g_camGetPos(charTransform, pos);
        g_charWorldPos = {pos[0], pos[1], pos[2]};
        
        if (g_camGetRot) {
          float q[4] = {};
          g_camGetRot(charTransform, q);  
          
          g_charYaw = atan2f(2.0f * (q[3] * q[1] + q[0] * q[2]),
                             1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]));
        }
        Log("[CAM] Character world pos: (%.3f, %.3f, %.3f) yaw=%.1fdeg",
            g_charWorldPos.x, g_charWorldPos.y, g_charWorldPos.z,
            g_charYaw * 57.2958f);

        
        
        
        
        g_charHeight = 0.0f;
        if (g_animator_GetBoneTransform && g_cachedAnimator) {
          void *headT = SafeGetBoneTransform(10); 
          Vec3 headPos;
          if (headT && ReadWorldPosition(headT, headPos)) {
            float h = headPos.y - g_charWorldPos.y; 
            if (h > 0.1f && h < 5.0f) 
              g_charHeight = h;
          }
        }
        if (g_charHeight > 0.0f) {
          
          
          
          if (g_camRefHeight <= 0.0f)
            g_camRefHeight = (CAM_REF_HEIGHT > 0.0f) ? CAM_REF_HEIGHT
                                                     : g_charHeight;
          g_camHeightScale = g_charHeight / g_camRefHeight;
        } else {
          g_camHeightScale = 1.0f; 
        }
        Log("[CAM] Char height=%.3f refHeight=%.3f heightScale=%.3f",
            g_charHeight, g_camRefHeight, g_camHeightScale);
      }
    } __except (1) {
      Log("[CAM] Failed to capture character world pos");
    }
    CaptureAndDisableCinemachine();
    g_cameraActive = true;
  }

  
  
  
  if (g_cameraActive && g_musclePlayer) {
    ApplyCameraFrame(g_musclePlayer->currentTime);
  }
}


static void ReapplyBoneTransforms() {
  if (!g_bonesReady)
    return;

  
  

  for (int b = 0; b < MAX_HUMAN_BONES; b++) {
    if (!g_cachedBones[b].valid || !g_cachedBones[b].transform)
      continue;
    SafeSetLocalRotation(g_cachedBones[b].transform, g_cachedBones[b].rotation);
  }

  
  if (g_hipsTransform) {
    SafeSetLocalPosition(g_hipsTransform, g_cachedHipsPos);
  }
}


static LRESULT CALLBACK MmdWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                   LPARAM lParam) {
  if (msg == WM_MMD_APPLY_POSE) {
    static bool s_firstLog = false;
    if (!s_firstLog) {
      Log("[WNDPROC] WM_MMD_APPLY_POSE received on main thread!");
      s_firstLog = true;
    }
    
    if (!s_poseReady) {
      InitMmdPoseOnMainThread();
    }
    if (s_poseReady) {
      ApplyMmdPoseOnMainThread();
    }
    g_mmdPendingApply = false;
    return 0;
  }
  return CallWindowProcW(g_origWndProc, hwnd, msg, wParam, lParam);
}


static void MuscleAnimationTick() {
  if (!g_musclePlayer || !g_musclePlayer->playing)
    return;
  if (!g_muscleAnim || !g_muscleAnim->loaded)
    return;

  
  
  
  if (g_mmdPendingApply)
    return;

  float frameNum =
      g_musclePlayer->Tick(); 
  float timeSec =
      frameNum / g_muscleAnim->fps; 
  MuscleFrame mf = g_muscleAnim->GetFrame(timeSec);

  
  memcpy((void *)g_mmdMuscles, mf.muscles, 95 * sizeof(float));
  memcpy((void *)g_mmdBodyPos, mf.bodyPos, 3 * sizeof(float));
  memcpy((void *)g_mmdBodyRot, mf.bodyRot, 4 * sizeof(float));
  if (g_muscleAnim->hasArmBones) {
    memcpy((void *)g_mmdArmBoneRots, mf.armBoneRots,
           ARM_BONE_COUNT * 4 * sizeof(float));
    g_mmdHasArmBones = true;
  }
  if (g_muscleAnim->hasFingerBones) {
    memcpy((void *)g_mmdFingerBoneRots, mf.fingerBoneRots,
           FINGER_BONE_COUNT * 4 * sizeof(float));
    g_mmdHasFingerBones = true;
  }
  g_mmdHasMuscles = true;

  
  
  if (!g_vmd && !g_bsIndicesResolved) {
    g_bsIndicesResolved = true; 
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA("plugin\\*.vmd", &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
      
      bool found = false;
      do {
        if (_stricmp(fd.cFileName, "camera.vmd") != 0) {
          found = true;
          break;
        }
      } while (FindNextFileA(hFind, &fd));
      FindClose(hFind);
      if (found) {
      char vmdPath[MAX_PATH];
      snprintf(vmdPath, sizeof(vmdPath), "plugin\\%s", fd.cFileName);
      Log("[MOUTH] Auto-loading VMD for morph data: %s", vmdPath);
      g_vmd = LoadVmd(vmdPath);
      if (g_vmd && g_vmd->loaded) {
        Log("[MOUTH] VMD loaded: %d morph timelines, %u frames",
            (int)g_vmd->morphTimelines.size(), g_vmd->totalFrames);
      } else {
        Log("[MOUTH] VMD load failed");
      }
      } else {
        Log("[MOUTH] Only camera.vmd found, no morph VMD");
      }
    } else {
      Log("[MOUTH] No .vmd file found in plugin/ dir for morph data");
    }
  }

  
  if (g_vmd && g_vmd->loaded && !g_vmd->morphTimelines.empty()) {
    
    
    static const char *morphNames[5] = {
        "\xe3\x81\x82", 
        "\xe3\x81\x84", 
        "\xe3\x81\x86", 
        "\xe3\x81\x88", 
        "\xe3\x81\x8a", 
    };

    static bool s_morphMapped = false;
    if (!s_morphMapped) {
      s_morphMapped = true;
      static const char *labelNames[5] = {"A", "I", "U", "E", "O"};
      Log("[MOUTH] VMD morph timelines: %d total",
          (int)g_vmd->morphTimelines.size());
      for (int i = 0; i < 5; i++) {
        auto it = g_vmd->morphTimelines.find(morphNames[i]);
        if (it != g_vmd->morphTimelines.end()) {
          float peak = 0;
          for (auto &k : it->second.keys)
            if (k.weight > peak)
              peak = k.weight;
          Log("[MOUTH]   %s: %d keys (peak=%.2f)", labelNames[i],
              (int)it->second.keys.size(), peak);
        } else {
          Log("[MOUTH]   %s: NOT FOUND in VMD", labelNames[i]);
        }
      }

      
      for (auto &pair : g_vmd->morphTimelines) {
        float peak = 0;
        float firstW = pair.second.keys.empty() ? 0 : pair.second.keys.front().weight;
        for (auto &k : pair.second.keys)
          if (k.weight > peak) peak = k.weight;
        Log("[MOUTH-ALL]   %s: %d keys (peak=%.2f, frame0=%.2f)",
            pair.first.c_str(), (int)pair.second.keys.size(), peak, firstW);
      }
    }

    
    static float s_prevMouthWeights[5] = {};
    static float s_mouthAlpha =
        0.25f; 

    for (int i = 0; i < 5; i++) {
      float target = 0.0f;
      auto it = g_vmd->morphTimelines.find(morphNames[i]);
      if (it != g_vmd->morphTimelines.end()) {
        target = it->second.Sample(frameNum);
      }
      
      float smoothed = s_prevMouthWeights[i] +
                       s_mouthAlpha * (target - s_prevMouthWeights[i]);
      s_prevMouthWeights[i] = smoothed;
      g_mouthWeights[i] = smoothed;
    }
    g_mouthWeightsFromMuscle = true;

    
    static float s_extraAlpha = 0.25f;
    for (int em = 0; em < NUM_EXTRA_MORPHS; em++) {
      float target = 0.0f;
      auto it = g_vmd->morphTimelines.find(g_extraMorphs[em].vmdNameUtf8);
      if (it != g_vmd->morphTimelines.end()) {
        target = it->second.Sample(frameNum);
      }
      float smoothed = g_extraMorphs[em].prevWeight +
                       s_extraAlpha * (target - g_extraMorphs[em].prevWeight);
      g_extraMorphs[em].prevWeight = smoothed;
      g_extraMorphs[em].weight = smoothed;
    }
  }

  
  if (g_gameHwnd) {
    g_mmdPendingApply = true;
    PostMessageW(g_gameHwnd, WM_MMD_APPLY_POSE, 0, 0);
  }
  
  

  static bool s_logged = false;
  if (!s_logged) {
    Log("[MUSCLE] Hotkey thread: pure SetHumanPose mode. m[0]=%.3f t=%.2f "
        "ready=%d",
        mf.muscles[0], timeSec, s_poseReady ? 1 : 0);
    s_logged = true;
  }
}

static void AnimationTick() {
  if (g_calibMode) {
    SafeSetAnimatorEnabled(false); 
    CalibrationTick();
    return;
  }
  if (!g_player || !g_player->playing)
    return;
  if (!g_vmd || !g_vmd->loaded)
    return;
  if (!g_resolvedMappings || g_resolvedMappings->empty())
    return;

  
  SafeSetAnimatorEnabled(false);

  float frame = g_player->Tick();

  
  FILE *dumpF = nullptr;
  if (!s_dumpedFrame0 && frame < 1.0f) {
    s_dumpedFrame0 = true;
    dumpF = fopen("plugin\\endfield_mmd_frame0.txt", "w");
    if (dumpF)
      fprintf(dumpF, "=== VMD Frame 0 Dump ===\n\n");
  }

  for (auto &rm : *g_resolvedMappings) {
    if (!rm.valid || !rm.transform)
      continue;
    CaptureBindPose(rm);
    auto it = g_vmd->boneTimelines.find(rm.mmdName);
    if (it == g_vmd->boneTimelines.end())
      continue;
    InterpResult interp =
        InterpolateBone(it->second.keys, frame, rm.isPositionBone);

    
    
    
    
    
    
    Quat raw = interp.rotation;
    Quat R_bip = rm.hasBind ? Quat{rm.bindRot[0], rm.bindRot[1], rm.bindRot[2],
                                   rm.bindRot[3]}
                            : Quat{0, 0, 0, 1};
    Quat R_mmd = GetMmdRestRot(rm.humanBone);

    Quat result;
    int mode = g_corrMode % g_corrCount;
    switch (mode) {
    case 0: {
      
      result = QuatMul(QuatMul(R_bip, QuatInv(R_mmd)), raw);
    } break;
    case 1: {
      
      Quat vmd_flipped = {raw.x, raw.y, -raw.z, -raw.w};
      result = QuatMul(QuatMul(R_bip, QuatInv(R_mmd)), vmd_flipped);
    } break;
    case 2: {
      
      Quat vmd_flipped = {-raw.x, -raw.y, raw.z, raw.w};
      result = QuatMul(QuatMul(R_bip, QuatInv(R_mmd)), vmd_flipped);
    } break;
    case 3: {
      
      result = QuatMul(raw, QuatMul(R_bip, QuatInv(R_mmd)));
    } break;
    case 4: {
      
      result = QuatMul(QuatMul(QuatInv(R_mmd), R_bip), raw);
    } break;
    case 5: {
      
      static const Quat G = {0.5f, -0.5f, 0.5f, 0.5f};
      static const Quat Ginv = {-0.5f, 0.5f, -0.5f, 0.5f};
      Quat sim_raw = QuatMul(QuatMul(G, raw), Ginv);
      result = QuatMul(QuatMul(R_bip, QuatInv(R_mmd)), sim_raw);
    } break;
    case 6: {
      
      static const Quat G = {0.5f, -0.5f, 0.5f, 0.5f};
      static const Quat Ginv = {-0.5f, 0.5f, -0.5f, 0.5f};
      result = QuatMul(R_bip, QuatMul(QuatMul(G, raw), Ginv));
    } break;
    case 7: {
      
      result = R_bip;
    } break;
    default:
      result = QuatMul(QuatMul(R_bip, QuatInv(R_mmd)), raw);
      break;
    }
    SafeSetLocalRotation(rm.transform, result);

    if (interp.hasPosition) {
      Vec3 vmdPos = MmdPosToUnity(interp.position);
      Vec3 fp = {rm.bindPos[0] + vmdPos.x, rm.bindPos[1] + vmdPos.y,
                 rm.bindPos[2] + vmdPos.z};
      SafeSetLocalPosition(rm.transform, fp);
    }
    
    if (dumpF) {
      fprintf(dumpF,
              "%-20s hb=%2d  vmd(%7.4f,%7.4f,%7.4f,%7.4f)  "
              "R_bip(%7.4f,%7.4f,%7.4f,%7.4f)  R_mmd(%7.4f,%7.4f,%7.4f,%7.4f)  "
              "out(%7.4f,%7.4f,%7.4f,%7.4f)  mode=%d\n",
              rm.mmdName.c_str(), rm.humanBone, raw.x, raw.y, raw.z, raw.w,
              R_bip.x, R_bip.y, R_bip.z, R_bip.w, R_mmd.x, R_mmd.y, R_mmd.z,
              R_mmd.w, result.x, result.y, result.z, result.w, mode);
    }
  }
  if (dumpF) {
    fclose(dumpF);
    dumpF = nullptr;
  }
}

