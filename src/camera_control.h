#pragma once

static void ListComponentsOnGameObject(void *go, const char *tag) {
  if (!go) return;
  __try {
    void *getCompMethod =
        FindMethod(il2cpp_object_get_class(go), "GetComponents", 1);
    if (!getCompMethod) {
      Log("[CAM-PROBE] %s: GetComponents method not found", tag);
      return;
    }
    void *compType = il2cpp_class_get_type(g_componentClass);
    void *typeObj = compType ? il2cpp_type_get_object(compType) : nullptr;
    if (!typeObj) {
      Log("[CAM-PROBE] %s: Component typeObj null", tag);
      return;
    }
    void *args[] = {typeObj};
    void *arr = Invoke(getCompMethod, go, args);
    if (!arr) {
      Log("[CAM-PROBE] %s: GetComponents returned null", tag);
      return;
    }
    int cnt = *(int *)((char *)arr + 24);
    void **data = (void **)((char *)arr + 32);
    Log("[CAM-PROBE] %s: %d components", tag, cnt);
    for (int i = 0; i < cnt; i++) {
      if (!data[i]) continue;
      void *cls = il2cpp_object_get_class(data[i]);
      const char *clsName = cls ? il2cpp_class_get_name(cls) : "?";
      const char *clsNs = cls ? il2cpp_class_get_namespace(cls) : "";
      Log("[CAM-PROBE]   [%d] %s.%s @ %p", i, (clsNs && clsNs[0]) ? clsNs : "-",
          clsName ? clsName : "?", data[i]);
    }
  } __except (1) {
    Log("[CAM-PROBE] %s: exception during component enumeration", tag);
  }
}

static void InvestigateCamera() {
  Log("[CAM-PROBE] ===== Camera system investigation start =====");
  if (!g_camera_get_main) {
    Log("[CAM-PROBE] Camera.get_main not resolved, abort");
    return;
  }

  void *mainCam = Invoke(g_camera_get_main, nullptr);
  if (!mainCam) {
    Log("[CAM-PROBE] Camera.main returned null (no main camera tagged?)");
    return;
  }
  Log("[CAM-PROBE] Camera.main = %p", mainCam);

  if (g_camera_get_fieldOfView) {
    void *fovBox = Invoke(g_camera_get_fieldOfView, mainCam);
    if (fovBox) {
      float fov = *(float *)((char *)fovBox + 16);
      Log("[CAM-PROBE] Current FOV = %.2f", fov);
    }
  }

  void *camGO = g_component_get_gameObject
                    ? Invoke(g_component_get_gameObject, mainCam)
                    : nullptr;
  if (camGO) {
    void *nameStr = g_object_get_name ? Invoke(g_object_get_name, camGO) : nullptr;
    char goName[128] = "";
    if (nameStr) ReadStrUtf8(nameStr, goName, sizeof(goName));
    Log("[CAM-PROBE] Camera GO name = '%s'", goName);
    ListComponentsOnGameObject(camGO, "CameraGO");
  }

  void *camTransform = g_component_get_transform
                           ? Invoke(g_component_get_transform, mainCam)
                           : nullptr;
  int depth = 0;
  void *cur = camTransform;
  while (cur && depth < 6) {
    void *parent = g_transform_get_parent ? Invoke(g_transform_get_parent, cur)
                                          : nullptr;
    if (!parent) break;
    void *parentGO = g_component_get_gameObject
                         ? Invoke(g_component_get_gameObject, parent)
                         : nullptr;
    if (parentGO) {
      void *nameStr =
          g_object_get_name ? Invoke(g_object_get_name, parentGO) : nullptr;
      char goName[128] = "";
      if (nameStr) ReadStrUtf8(nameStr, goName, sizeof(goName));
      char tag[160];
      sprintf(tag, "Parent[%d] '%s'", depth, goName);
      ListComponentsOnGameObject(parentGO, tag);
    }
    cur = parent;
    depth++;
  }
  Log("[CAM-PROBE] ===== Camera system investigation end =====");
}

static void *g_mainCamera = nullptr;        
static void *g_mainCamTransform = nullptr;  
static void *g_camRootTransform = nullptr;  

static bool ResolveMainCamera() {
  if (!g_camera_get_main) return false;
  void *mainCam = Invoke(g_camera_get_main, nullptr);
  if (!mainCam) {
    Log("[CAM] Camera.main is null");
    return false;
  }
  g_mainCamera = mainCam;
  g_mainCamTransform = g_component_get_transform
                           ? Invoke(g_component_get_transform, mainCam)
                           : nullptr;

  g_camRootTransform = nullptr;
  if (g_mainCamTransform && g_transform_get_parent) {
    g_camRootTransform = Invoke(g_transform_get_parent, g_mainCamTransform);
    if (g_camRootTransform)
      Log("[CAM] CameraRoot transform: %p", g_camRootTransform);
  }

  s_cinemachineBrain = nullptr;
  __try {
    void *camGO = g_component_get_gameObject
                      ? Invoke(g_component_get_gameObject, mainCam)
                      : nullptr;
    if (camGO) {
      void *getCompMethod =
          FindMethod(il2cpp_object_get_class(camGO), "GetComponents", 1);
      void *compType = il2cpp_class_get_type(g_componentClass);
      void *typeObj = compType ? il2cpp_type_get_object(compType) : nullptr;
      if (getCompMethod && typeObj) {
        void *args[] = {typeObj};
        void *arr = Invoke(getCompMethod, camGO, args);
        if (arr) {
          int cnt = *(int *)((char *)arr + 24);
          void **data = (void **)((char *)arr + 32);
          for (int i = 0; i < cnt; i++) {
            if (!data[i]) continue;
            void *cls = il2cpp_object_get_class(data[i]);
            const char *cn = cls ? il2cpp_class_get_name(cls) : "";
            if (cn && strcmp(cn, "CinemachineBrain") == 0) {
              s_cinemachineBrain = data[i];
              Log("[CAM] CinemachineBrain found: %p", s_cinemachineBrain);
              break;
            }
          }
        }
      }
    }
  } __except (1) {
    Log("[CAM] exception finding CinemachineBrain");
  }
  return g_mainCamera != nullptr && g_mainCamTransform != nullptr;
}

static void CaptureAndDisableCinemachine() {
  if (!ResolveMainCamera()) {
    Log("[CAM] ResolveMainCamera failed, camera takeover aborted");
    return;
  }
  if (g_camera_get_fieldOfView) {
    void *fovBox = Invoke(g_camera_get_fieldOfView, g_mainCamera);
    if (fovBox) g_origFov = *(float *)((char *)fovBox + 16);
  }
  if (s_cinemachineBrain && g_animator_set_enabled) {
    __try {
      int falseVal = 0;
      void *params[] = {&falseVal};
      Invoke(g_animator_set_enabled, s_cinemachineBrain, params);
      Log("[CAM] CinemachineBrain DISABLED (origFov=%.2f)", g_origFov);
    } __except (1) {
      Log("[CAM] Failed to disable CinemachineBrain");
    }
  } else {
    Log("[CAM] CinemachineBrain not found — camera may be overridden each frame");
  }
}

static void RestoreCinemachine() {
  if (s_cinemachineBrain && g_animator_set_enabled) {
    __try {
      int trueVal = 1;
      void *params[] = {&trueVal};
      Invoke(g_animator_set_enabled, s_cinemachineBrain, params);
      Log("[CAM] CinemachineBrain RE-ENABLED");
    } __except (1) {
    }
  }
  if (g_mainCamera && g_camera_set_fieldOfView && g_origFov > 0.0f) {
    __try {
      void *params[] = {&g_origFov};
      Invoke(g_camera_set_fieldOfView, g_mainCamera, params);
    } __except (1) {
    }
  }
  s_cinemachineBrain = nullptr;
  g_mainCamera = nullptr;
  g_mainCamTransform = nullptr;
  g_camRootTransform = nullptr;
}


static Vec3 SampleCharDisplacement(float timeSec) {
  Vec3 disp = {0, 0, 0};
  if (!g_vmd) return disp;
  auto it = g_vmd->boneTimelines.find(
      "\xe3\x82\xbb\xe3\x83\xb3\xe3\x82\xbf\xe3\x83\xbc"); 
  if (it != g_vmd->boneTimelines.end()) {
    float frameF = timeSec * 30.0f;
    InterpResult ir = InterpolateBone(it->second.keys, frameF, true);
    disp = ir.position;
  }
  return disp;
}

static void ApplyCameraFrame(float timeSec) {
  if (!g_cameraActive || !g_mainCamera || !g_mainCamTransform)
    return;
  if (!g_camTestMode && !g_cameraPlayer.HasData())
    return;

  CameraState cs;
  if (g_camTestMode) {
    cs.position = {0, 0, 0};
    cs.rotation = {0, 0, 0, 1};
    cs.fov = 40.0f;
    cs.valid = true;
  } else {
    cs = g_cameraPlayer.Sample(timeSec);
  }
  if (!cs.valid) return;

  float effYaw = CAM_YAW_SIGN * g_charYaw + CAM_YAW_BIAS * 0.0174533f;

  Vec3 charDisp = SampleCharDisplacement(timeSec);

  float camScale = g_cameraPlayer.scale;
  float hs = g_camHeightScale + g_camHeightBias;
  float px = (cs.position.x - charDisp.x * camScale) * hs;
  float py = cs.position.y * hs;
  float pz = (cs.position.z - charDisp.z * camScale) * hs;
  float cy = cosf(effYaw), sy = sinf(effYaw);
  float rx = px * cy + pz * sy;
  float rz = -px * sy + pz * cy;
  float worldPos[3] = {rx + g_charWorldPos.x,
                       py + g_charWorldPos.y,
                       rz + g_charWorldPos.z};

  float toCharX = g_charWorldPos.x - worldPos[0];
  float toCharZ = g_charWorldPos.z - worldPos[2];
  float lookYaw = atan2f(toCharX, toCharZ);

  float pitch = CAM_SIGN_RX * cs.euler.x;
  float roll  = CAM_SIGN_RZ * cs.euler.z;
  float hp = pitch * 0.5f, hr = roll * 0.5f, hly = lookYaw * 0.5f;
  Quat qLookYaw = {0, sinf(hly), 0, cosf(hly)};
  Quat qPitch   = {sinf(hp), 0, 0, cosf(hp)};
  Quat qRoll    = {0, 0, sinf(hr), cosf(hr)};
  Quat finalRot = QuatMul(QuatMul(qLookYaw, qPitch), qRoll);
  float worldRot[4] = {finalRot.x, finalRot.y, finalRot.z, finalRot.w};

  static int s_camDiag = 0;

  if (g_camTestMode) {
    static LARGE_INTEGER s_freq = {}, s_start = {};
    if (s_freq.QuadPart == 0) {
      QueryPerformanceFrequency(&s_freq);
      QueryPerformanceCounter(&s_start);
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    float elapsed = (float)(now.QuadPart - s_start.QuadPart) / s_freq.QuadPart;
    float ang = elapsed * 0.5f;  
    float r = 5.0f;
    worldPos[0] = g_charWorldPos.x + r * sinf(ang);
    worldPos[1] = g_charWorldPos.y;
    worldPos[2] = g_charWorldPos.z + r * cosf(ang);
    float half = (ang + 3.14159f) * 0.5f;
    worldRot[0] = 0.0f;
    worldRot[1] = sinf(half);
    worldRot[2] = 0.0f;
    worldRot[3] = cosf(half);
  }

  g_camHookTransform = g_mainCamTransform;
  __try {
    g_camSelfWrite = true;
    if (g_origSetLocalPos) g_origSetLocalPos(g_mainCamTransform, worldPos);
    else if (g_nativeSetPos) g_nativeSetPos(g_mainCamTransform, worldPos);
    if (g_origSetLocalRot) g_origSetLocalRot(g_mainCamTransform, worldRot);
    else if (g_nativeSetRot) g_nativeSetRot(g_mainCamTransform, worldRot);
    g_camSelfWrite = false;
    if (g_camera_set_fieldOfView) {
      void *args[] = {&cs.fov};
      Invoke(g_camera_set_fieldOfView, g_mainCamera, args);
    }

    s_camDiag++;
    if (s_camDiag <= 3 || s_camDiag % 60 == 0) {
      float readPos[3] = {}, readRot[4] = {};
      if (g_camGetPos) g_camGetPos(g_mainCamTransform, readPos);
      if (g_camGetRot) g_camGetRot(g_mainCamTransform, readRot);
      Log("[CAM-DIAG] #%d t=%.1f pos=(%.2f,%.2f,%.2f) rot=(%.3f,%.3f,%.3f,%.3f) fov=%.1f",
          s_camDiag, timeSec,
          readPos[0], readPos[1], readPos[2],
          readRot[0], readRot[1], readRot[2], readRot[3], cs.fov);
      Log("[CAM-DIAG] #%d VMDrot=(%.3f,%.3f,%.3f,%.3f)",
          s_camDiag, worldRot[0], worldRot[1], worldRot[2], worldRot[3]);
    }
  } __except (1) {
  }
}

