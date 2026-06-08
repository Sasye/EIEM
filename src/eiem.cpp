#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <commdlg.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "vmd_parser.h"
#include "mmd_player.h"
#include "bone_anim_player.h"
#include "bone_map.h"
#include "il2cpp_api.h"
#include "muscle_player.h"
#include "camera_player.h"


#include "globals.h"

#include "audio.h"

#include "smc_face.h"



static int32_t UnboxInt(void *boxed) {
  __try {
    return boxed ? *(int32_t *)((char *)boxed + 16) : 0;
  } __except (1) {
    return 0;
  }
}
static bool UnboxBool(void *boxed) {
  __try {
    return boxed ? *(bool *)((char *)boxed + 16) : false;
  } __except (1) {
    return false;
  }
}

#include "animation.h"
#include "trojan.h"
#include "gui.h"
#include "init.h"




static HANDLE g_initThread = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    g_initThread = CreateThread(NULL, 0, InitThread, NULL, 0, NULL);
  }
  return TRUE;
}
