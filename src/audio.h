#pragma once








#include <windows.h>
#include <mmsystem.h>
#include <cstdio>
#include <cstring>


void Log(const char *fmt, ...);

struct AudioPlayer {
  bool loaded = false;
  bool playing = false;   
  bool isMp3 = false;     
  int lengthMs = 0;       

  
  static bool Mci(const char *cmd) {
    MCIERROR err = mciSendStringA(cmd, nullptr, 0, nullptr);
    if (err != 0) {
      char errBuf[256] = {};
      mciGetErrorStringA(err, errBuf, sizeof(errBuf));
      Log("[AUDIO] MCI cmd failed: \"%s\" -> %s", cmd, errBuf);
      return false;
    }
    return true;
  }

  
  static int MciQuery(const char *cmd) {
    char ret[64] = {};
    MCIERROR err = mciSendStringA(cmd, ret, sizeof(ret), nullptr);
    if (err != 0)
      return -1;
    return atoi(ret);
  }

  
  bool Open(const char *path) {
    Close();
    if (!path || path[0] == '\0')
      return false;

    
    const char *ext = strrchr(path, '.');
    isMp3 = (ext && _stricmp(ext, ".mp3") == 0);
    
    
    const char *devType = "mpegvideo";

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "open \"%s\" type %s alias eiem_bgm", path, devType);
    if (!Mci(cmd)) {
      
      Log("[AUDIO] Open failed: %s (%s)", path, devType);
      return false;
    }

    
    Mci("set eiem_bgm time format milliseconds");
    lengthMs = MciQuery("status eiem_bgm length");
    if (lengthMs < 0)
      lengthMs = 0;
    loaded = true;
    playing = false;
    Log("[AUDIO] Opened %s (%s), length=%d ms", path, devType, lengthMs);
    return true;
  }

  void Play() {
    if (!loaded)
      return;
    if (Mci("play eiem_bgm from 0"))
      playing = true;
  }

  void PlayFrom(int ms) {
    if (!loaded)
      return;
    if (ms < 0)
      ms = 0;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "play eiem_bgm from %d", ms);
    if (Mci(cmd))
      playing = true;
  }

  void Pause() {
    if (!loaded || !playing)
      return;
    Mci("pause eiem_bgm");
    playing = false;
  }

  void Resume() {
    if (!loaded)
      return;
    if (Mci("resume eiem_bgm"))
      playing = true;
  }

  void Stop() {
    if (!loaded)
      return;
    Mci("stop eiem_bgm");
    Mci("seek eiem_bgm to start");
    playing = false;
  }

  void Close() {
    if (loaded) {
      Mci("close eiem_bgm");
      Log("[AUDIO] Closed");
    }
    loaded = false;
    playing = false;
    lengthMs = 0;
  }

  
  int GetPositionMs() {
    if (!loaded)
      return -1;
    return MciQuery("status eiem_bgm position");
  }

  int GetLengthMs() const { return lengthMs; }

  
  bool AtEnd() {
    if (!loaded || lengthMs <= 0)
      return false;
    int pos = GetPositionMs();
    if (pos < 0)
      return false;
    return pos >= lengthMs - 30; 
  }

  
  void SetVolume(int vol) {
    if (!loaded)
      return;
    if (vol < 0) vol = 0;
    if (vol > 1000) vol = 1000;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "setaudio eiem_bgm volume to %d", vol);
    Mci(cmd);
  }
};
