#pragma once






#include <d3d11.h>
#include <dxgi1_2.h>
#include <dwmapi.h>
#include <dcomp.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dcomp.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);




#define WM_MMD_GUI_PLAY   (WM_USER + 100)
#define WM_MMD_GUI_PAUSE  (WM_USER + 101)
#define WM_MMD_GUI_STOP   (WM_USER + 102)
#define WM_MMD_GUI_LOAD   (WM_USER + 103)

#define WM_MMD_GUI_RECAPTURE (WM_USER + 106)
#define WM_MMD_GUI_LOAD_AUDIO (WM_USER + 107)
#define WM_MMD_GUI_SEEK_AUDIO (WM_USER + 108) 
#define WM_MMD_GUI_SET_VOLUME (WM_USER + 109) 





static volatile bool g_guiRunning = false;















typedef void* (*tCreateBindingByActionId)(void* __this, void* actionId,
                                          void* callback, int priority,
                                          void* methodInfo);
static tCreateBindingByActionId g_origCreateBinding = nullptr;



static void ResolveActionInvoke(void* actionObj) {
  if (g_actionInvokeMethod || !actionObj) return;
  void* klass = il2cpp_object_get_class(actionObj);
  if (!klass) return;
  void* it = nullptr;
  void* m;
  while ((m = il2cpp_class_get_methods(klass, &it))) {
    if (strcmp(il2cpp_method_get_name(m), "Invoke") == 0 &&
        il2cpp_method_get_param_count(m) == 0) {
      g_actionInvokeMethod = m;
      Log("[CURSOR] Resolved Action.Invoke()");
      return;
    }
  }
}

static void InvokeAction(void* actionObj) {
  if (!actionObj) return;
  if (!g_actionInvokeMethod) ResolveActionInvoke(actionObj);
  if (!g_actionInvokeMethod) return;
  __try {
    void* exc = nullptr;
    il2cpp_runtime_invoke(g_actionInvokeMethod, actionObj, nullptr, &exc);
  } __except (1) {
    Log("[CURSOR] SEH in InvokeAction (delegate may be GC'd)");
  }
}

static void* hkCreateBindingByActionId(void* __this, void* actionId,
                                        void* callback, int priority,
                                        void* methodInfo) {
  void* result = g_origCreateBinding(__this, actionId, callback, priority,
                                      methodInfo);
  
  char buf[256];
  if (ReadStr(actionId, buf, sizeof(buf)) > 0) {
    
    
    
    if (strcmp(buf, "common_show_cursor_start") == 0) {
      g_cursorShowAction = callback;
      if (il2cpp_gchandle_new) il2cpp_gchandle_new(callback, true);
      if (!g_actionInvokeMethod) ResolveActionInvoke(callback);
      Log("[CURSOR] CAPTURED show-cursor action: %p (invoke=%p)", callback,
          g_actionInvokeMethod);
    } else if (strcmp(buf, "common_show_cursor_end") == 0) {
      g_cursorHideAction = callback;
      if (il2cpp_gchandle_new) il2cpp_gchandle_new(callback, true);
      Log("[CURSOR] CAPTURED hide-cursor action: %p", callback);
    }
  }
  return result;
}

static void DumpCursorMethods() {
  if (!il2cpp_domain_get || !il2cpp_domain_get_assemblies) return;

  void *domain = il2cpp_domain_get();
  size_t asmCount = 0;
  void **assemblies = il2cpp_domain_get_assemblies(domain, &asmCount);

  
  for (size_t i = 0; i < asmCount; i++) {
    void *img = il2cpp_assembly_get_image(assemblies[i]);
    if (!img) continue;
    void *klass = il2cpp_class_from_name(img, "Beyond.Input", "InputManager");
    if (!klass) continue;

    Log("[CURSOR] Found Beyond.Input.InputManager");

    
    void *it = nullptr, *m;
    while ((m = il2cpp_class_get_methods(klass, &it))) {
      const char *mn = il2cpp_method_get_name(m);
      if (!mn) continue;
      uint32_t pc = il2cpp_method_get_param_count(m);

      if (strcmp(mn, "CreateBindingByActionId") == 0 && pc == 3) {
        void *mp = ((MInfo *)m)->mp;
        if (mp && MH_CreateHook(mp, (void *)hkCreateBindingByActionId,
                                (void **)&g_origCreateBinding) == MH_OK) {
          MH_EnableHook(mp);
          Log("[CURSOR] Hooked CreateBindingByActionId: %p", mp);
        }
      }
    }
    break;
  }
}

#define WM_MMD_CURSOR_SHOW  (WM_USER + 104)
#define WM_MMD_CURSOR_HIDE  (WM_USER + 105)


static void ReleaseCursorToGui() {
  if (g_gameHwnd && g_cursorShowAction)
    PostMessageW(g_gameHwnd, WM_MMD_CURSOR_SHOW, 0, 0);
}


static void ReturnCursorToGame() {
  if (g_gameHwnd && g_cursorHideAction)
    PostMessageW(g_gameHwnd, WM_MMD_CURSOR_HIDE, 0, 0);
}





static ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
static IDXGISwapChain1 *g_pSwapChain = nullptr;
static ID3D11RenderTargetView *g_pMainRenderTargetView = nullptr;
static IDCompositionDevice *g_pDCompDevice = nullptr;
static IDCompositionTarget *g_pDCompTarget = nullptr;
static IDCompositionVisual *g_pDCompVisual = nullptr;

static void CreateRenderTarget() {
  ID3D11Texture2D *pBackBuffer = nullptr;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  if (pBackBuffer) {
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
                                          &g_pMainRenderTargetView);
    pBackBuffer->Release();
  }
}

static void CleanupRenderTarget() {
  if (g_pMainRenderTargetView) {
    g_pMainRenderTargetView->Release();
    g_pMainRenderTargetView = nullptr;
  }
}

static bool CreateDeviceD3D(HWND hWnd) {
  
  UINT createDeviceFlags = 0;
  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[] = {D3D_FEATURE_LEVEL_11_0};
  HRESULT hr = D3D11CreateDevice(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
      featureLevelArray, 1, D3D11_SDK_VERSION,
      &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
  if (hr == DXGI_ERROR_UNSUPPORTED) {
    hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
        featureLevelArray, 1, D3D11_SDK_VERSION,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
  }
  if (FAILED(hr)) return false;

  
  IDXGIDevice *pDxgiDevice = nullptr;
  g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice));
  IDXGIAdapter *pAdapter = nullptr;
  pDxgiDevice->GetAdapter(&pAdapter);
  IDXGIFactory2 *pFactory = nullptr;
  pAdapter->GetParent(IID_PPV_ARGS(&pFactory));

  
  RECT rc;
  GetClientRect(hWnd, &rc);
  DXGI_SWAP_CHAIN_DESC1 sd = {};
  sd.Width = rc.right - rc.left;
  sd.Height = rc.bottom - rc.top;
  sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  sd.SampleDesc.Count = 1;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount = 2;
  sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  sd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
  
  hr = pFactory->CreateSwapChainForComposition(g_pd3dDevice, &sd, nullptr,
                                                &g_pSwapChain);
  pFactory->Release();
  pAdapter->Release();

  if (FAILED(hr)) {
    Log("[GUI] CreateSwapChainForComposition failed: 0x%08X", hr);
    pDxgiDevice->Release();
    return false;
  }

  
  hr = DCompositionCreateDevice(pDxgiDevice, IID_PPV_ARGS(&g_pDCompDevice));
  pDxgiDevice->Release();
  if (FAILED(hr)) {
    Log("[GUI] DCompositionCreateDevice failed: 0x%08X", hr);
    return false;
  }
  g_pDCompDevice->CreateTargetForHwnd(hWnd, TRUE, &g_pDCompTarget);
  g_pDCompDevice->CreateVisual(&g_pDCompVisual);
  g_pDCompVisual->SetContent(g_pSwapChain);
  g_pDCompTarget->SetRoot(g_pDCompVisual);
  g_pDCompDevice->Commit();

  CreateRenderTarget();
  Log("[GUI] DComp transparent swap chain created");
  return true;
}

static void CleanupDeviceD3D() {
  CleanupRenderTarget();
  if (g_pDCompVisual) { g_pDCompVisual->Release(); g_pDCompVisual = nullptr; }
  if (g_pDCompTarget) { g_pDCompTarget->Release(); g_pDCompTarget = nullptr; }
  if (g_pDCompDevice) { g_pDCompDevice->Release(); g_pDCompDevice = nullptr; }
  if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
  if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
  if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}




static LRESULT CALLBACK GuiWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg) {
  case WM_SIZE:
    if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
      CleanupRenderTarget();
      g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam),
                                   (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN,
                                   0);
      CreateRenderTarget();
    }
    return 0;
  case WM_MOVING:
    
    if (g_gameHwnd) {
      RECT gr;
      GetWindowRect(g_gameHwnd, &gr);
      RECT *pr = (RECT *)lParam;
      int pw = pr->right - pr->left;
      int ph = pr->bottom - pr->top;
      if (pr->left < gr.left) { pr->left = gr.left; pr->right = pr->left + pw; }
      if (pr->top < gr.top) { pr->top = gr.top; pr->bottom = pr->top + ph; }
      if (pr->right > gr.right) { pr->right = gr.right; pr->left = pr->right - pw; }
      if (pr->bottom > gr.bottom) { pr->bottom = gr.bottom; pr->top = pr->bottom - ph; }
    }
    return TRUE;
  case WM_CLOSE:
    
    ShowWindow(hWnd, SW_HIDE);
    g_guiVisible = false;
    return 0;
  case WM_DESTROY:
    return 0;
  }
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}




static void DrawMainPanel() {
  
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("EIEM", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoScrollbar);
  ImGui::PopStyleVar();

  
  const float titleH = 36.0f;
  const float btnSize = 22.0f;
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 winPos = ImGui::GetWindowPos();
  ImVec2 winSize = ImGui::GetWindowSize();

  
  dl->AddRectFilled(winPos, ImVec2(winPos.x + winSize.x, winPos.y + titleH),
                    IM_COL32(24, 24, 24, 180));
  
  dl->AddRectFilled(winPos, ImVec2(winPos.x + 4, winPos.y + titleH),
                    IM_COL32(255, 218, 0, 255));
  
  dl->AddLine(ImVec2(winPos.x, winPos.y + titleH),
              ImVec2(winPos.x + winSize.x, winPos.y + titleH),
              IM_COL32(100, 100, 105, 80), 1.0f);

  
  dl->AddText(ImVec2(winPos.x + 14, winPos.y + 9), IM_COL32(255, 218, 0, 255),
              "EIEM");
  dl->AddText(ImVec2(winPos.x + 130, winPos.y + 9),
              IM_COL32(160, 160, 165, 255), u8"\u63a7\u5236\u9762\u677f");

  
  ImGui::SetCursorPos(ImVec2(0, 0));
  ImGui::InvisibleButton("##titlebar_drag",
                         ImVec2(winSize.x - btnSize - 16, titleH));
  
  
  static bool s_dragging = false;
  static POINT s_dragAnchor = {0, 0};
  static RECT s_winAtDrag = {0, 0, 0, 0};
  if (ImGui::IsItemActivated()) {
    s_dragging = true;
    GetCursorPos(&s_dragAnchor);
    GetWindowRect(g_guiHwnd, &s_winAtDrag);
  }
  if (s_dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    POINT cur;
    GetCursorPos(&cur);
    int nx = s_winAtDrag.left + (cur.x - s_dragAnchor.x);
    int ny = s_winAtDrag.top + (cur.y - s_dragAnchor.y);
    
    if (g_gameHwnd) {
      RECT gr;
      GetWindowRect(g_gameHwnd, &gr);
      int w = s_winAtDrag.right - s_winAtDrag.left;
      int h = s_winAtDrag.bottom - s_winAtDrag.top;
      if (nx < gr.left) nx = gr.left;
      if (ny < gr.top) ny = gr.top;
      if (nx + w > gr.right) nx = gr.right - w;
      if (ny + h > gr.bottom) ny = gr.bottom - h;
    }
    SetWindowPos(g_guiHwnd, nullptr, nx, ny, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  } else {
    s_dragging = false;
  }

  
  
  ImVec2 closePos = ImVec2(winSize.x - btnSize - 8, (titleH - btnSize) * 0.5f);
  ImGui::SetCursorPos(closePos);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, btnSize * 0.5f);
  ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(55, 58, 70, 255));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(210, 60, 60, 255));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(160, 35, 35, 255));
  bool closeClicked = ImGui::Button("##close", ImVec2(btnSize, btnSize));
  
  ImVec2 bMin = ImGui::GetItemRectMin();
  ImVec2 bMax = ImGui::GetItemRectMax();
  ImVec2 c = ImVec2((bMin.x + bMax.x) * 0.5f, (bMin.y + bMax.y) * 0.5f);
  float r = 5.0f;
  ImU32 xcol = IM_COL32(220, 220, 230, 255);
  dl->AddLine(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), xcol, 1.6f);
  dl->AddLine(ImVec2(c.x - r, c.y + r), ImVec2(c.x + r, c.y - r), xcol, 1.6f);
  ImGui::PopStyleColor(3);
  ImGui::PopStyleVar();
  if (closeClicked) {
    
    ShowWindow(g_guiHwnd, SW_HIDE);
    g_guiVisible = false;
    ReturnCursorToGame();
  }

  
  
  {
    ImVec2 gradTop = ImVec2(winPos.x, winPos.y + titleH);
    ImVec2 gradBot = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);
    ImU32 colTop = IM_COL32(255, 255, 255, 0);    
    ImU32 colBot = IM_COL32(255, 255, 255, 65);   
    dl->AddRectFilledMultiColor(gradTop, gradBot,
                                colTop, colTop,    
                                colBot, colBot);   
  }

  
  ImGui::SetCursorPos(ImVec2(10, titleH + 8));
  ImGui::BeginChild("##body", ImVec2(winSize.x - 20, winSize.y - titleH - 16),
                    false);

  ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None);

  
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
  bool tabCtrl = ImGui::BeginTabItem(u8"\u63a7\u5236");
  ImGui::PopStyleColor();
  if (tabCtrl) {

  
  ImGui::Spacing();
  bool isPlaying = false;
  bool isPaused = false;
  float curTime = 0.0f;
  float totalTime = 0.0f;
  int curFrame = 0;
  int totalFrames = 0;
  bool animLoaded = false;

  if (g_muscleAnim && g_muscleAnim->loaded) {
    animLoaded = true;
    totalTime = g_muscleAnim->Duration();
    totalFrames = g_muscleAnim->frameCount;
  }
  if (g_musclePlayer) {
    isPlaying = g_musclePlayer->playing;
    curTime = g_musclePlayer->currentTime;
    curFrame = (int)(curTime * 30.0f);
    isPaused = !isPlaying && curTime > 0.0f;
  }

  
  if (isPlaying) {
    ImGui::TextColored(ImVec4(0.10f, 0.55f, 0.25f, 1.0f), u8"\u64ad\u653e\u4e2d");
  } else if (isPaused) {
    ImGui::TextColored(ImVec4(0.80f, 0.55f, 0.00f, 1.0f), u8"\u5df2\u6682\u505c");
  } else {
    ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.48f, 1.0f), u8"\u5df2\u505c\u6b62");
  }

  
  ImGui::Text(u8"\u65f6\u95f4: %.1f / %.1f \u79d2", curTime, totalTime);
  ImGui::Text(u8"\u5e27: %d / %d", curFrame, totalFrames);

  
  if (totalTime > 0.0f && g_musclePlayer) {
    float seekTime = curTime;
    ImGui::SetNextItemWidth(-1);
    static bool s_wasDragging = false;
    static float s_dragTarget = 0.0f;
    if (ImGui::SliderFloat("##seek", &seekTime, 0.0f, totalTime, u8"%.1f \u79d2")) {
      
      g_musclePlayer->currentTime = seekTime;
      QueryPerformanceCounter(&g_musclePlayer->lastTick);
      s_wasDragging = true;
      s_dragTarget = seekTime;
    }
    
    if (s_wasDragging && !ImGui::IsItemActive()) {
      s_wasDragging = false;
      
      
      if (g_audioPlayer && g_audioPlayer->loaded && g_audioEnabled) {
        int ms = (int)(s_dragTarget * 1000.0f);
        Log("[AUDIO] Seek bar released -> posting seek %d ms", ms);
        PostMessageW(g_gameHwnd, WM_MMD_GUI_SEEK_AUDIO, (WPARAM)ms, 0);
      }
    }
  } else {
    float zero = 0.0f;
    ImGui::ProgressBar(zero, ImVec2(-1, 0), "");
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f); 
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f)); 
  float btnW = 76.0f;

  
  if (isPlaying) {
    ImGui::BeginDisabled();
    ImGui::Button(u8"\u64ad\u653e", ImVec2(btnW, 0));
    ImGui::EndDisabled();
  } else {
    if (ImGui::Button(u8"\u64ad\u653e", ImVec2(btnW, 0))) {
      PostMessageW(g_gameHwnd, WM_MMD_GUI_PLAY, 0, 0);
    }
  }
  ImGui::SameLine();

  
  if (!isPlaying) {
    ImGui::BeginDisabled();
    ImGui::Button(u8"\u6682\u505c", ImVec2(btnW, 0));
    ImGui::EndDisabled();
  } else {
    if (ImGui::Button(u8"\u6682\u505c", ImVec2(btnW, 0))) {
      PostMessageW(g_gameHwnd, WM_MMD_GUI_PAUSE, 0, 0);
    }
  }
  ImGui::SameLine();

  
  if (!isPlaying && !isPaused) {
    ImGui::BeginDisabled();
    ImGui::Button(u8"\u505c\u6b62", ImVec2(btnW, 0));
    ImGui::EndDisabled();
  } else {
    if (ImGui::Button(u8"\u505c\u6b62", ImVec2(btnW, 0))) {
      PostMessageW(g_gameHwnd, WM_MMD_GUI_STOP, 0, 0);
    }
  }
  ImGui::PopStyleColor(); 
  ImGui::PopStyleVar(); 

  ImGui::Spacing();

  
  
  
  if (g_musclePlayer) {
    g_playbackSpeed = g_musclePlayer->speed;
    g_playbackLoop  = g_musclePlayer->loop;
  }
  ImGui::SetNextItemWidth(140);
  ImGui::SliderFloat(u8"\u64ad\u653e\u901f\u5ea6", &g_playbackSpeed, 0.1f, 3.0f, u8"%.1fx");
  ImGui::SameLine();
  ImGui::Checkbox(u8"\u5faa\u73af", &g_playbackLoop);
  ImGui::SameLine();
  ImGui::Checkbox(u8"\u955c\u5934", &g_cameraEnabled);
  if (g_musclePlayer) {
    g_musclePlayer->speed = g_playbackSpeed;
    g_musclePlayer->loop  = g_playbackLoop;
  }

  
  ImGui::SetNextItemWidth(-1);
  ImGui::SliderFloat(u8"\u97f3\u9891\u504f\u79fb##offset", &g_audioOffset, -30.0f, 30.0f, u8"%.1f \u79d2");
  ImGui::TextDisabled(u8"\u6b63 = \u8df3\u8fc7\u97f3\u9891\u524d\u594f\uff0c\u8d1f = \u5ef6\u8fdf\u97f3\u9891");
  {
    int volPercent = g_audioVolume / 10;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt(u8"\u97f3\u91cf##vol", &volPercent, 0, 100, u8"%d%%")) {
      g_audioVolume = volPercent * 10;
      PostMessageW(g_gameHwnd, WM_MMD_GUI_SET_VOLUME, (WPARAM)g_audioVolume, 0);
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  
  ImGui::TextColored(ImVec4(0.20f, 0.20f, 0.24f, 1.0f), u8"\u52a8\u753b\u4fe1\u606f");
  if (animLoaded) {
    ImGui::Text(u8"\u6587\u4ef6: muscle_anim.bin");
    ImGui::Text(u8"Muscles: %d", g_muscleAnim->muscleCount);
    ImGui::Text(u8"FPS: %.0f", g_muscleAnim->fps);
    if (g_muscleAnim->hasFingerBones)
      ImGui::Text(u8"\u624b\u6307\u9aa8\u9abc: %d", g_muscleAnim->fingerBoneCount);
    else
      ImGui::TextDisabled(u8"\u624b\u6307\u9aa8\u9abc: \u65e0");
  } else {
    ImGui::TextDisabled(u8"\u672a\u52a0\u8f7d\u52a8\u753b");
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    if (ImGui::Button(u8"\u52a0\u8f7d", ImVec2(-1, 0))) {
      PostMessageW(g_gameHwnd, WM_MMD_GUI_LOAD, 0, 0);
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
  bool skirtOpen = ImGui::CollapsingHeader(u8"\u88d9\u5b50\u78b0\u649e\u8c03\u6574");
  ImGui::PopStyleColor();
  if (skirtOpen) {
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat(u8"\u5927\u817f\u6839\u534a\u5f84\u8865\u5145", &s_skirtHipRadiusDelta,
                           0.0f, 0.4f, "%.3f")) {
      s_skirtDirty = true; 
    }
    ImGui::TextDisabled(u8"\u539f\u59cb\u503c: 0.124  \u6548\u679c: \u52a0\u5927\u2192\u51cf\u5c11\u7a7f\u6a21");
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  
  ImGui::TextColored(ImVec4(0.20f, 0.20f, 0.24f, 1.0f), u8"\u7cfb\u7edf\u72b6\u6001");
  ImGui::Text(u8"Trojan: %s", g_trojanActive ? u8"\u6d3b\u52a8" : u8"\u7a7a\u95f2");
  ImGui::Text(u8"\u76f8\u673a: %s", g_cameraActive ? u8"\u6d3b\u52a8" : u8"\u5173\u95ed");
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
  if (ImGui::Button(u8"\u91cd\u65b0\u83b7\u53d6\u89d2\u8272", ImVec2(-1, 0))) {
    PostMessageW(g_gameHwnd, WM_MMD_GUI_RECAPTURE, 0, 0);
  }
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();

  ImGui::EndTabItem();
  } 

  
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
  bool tabFile = ImGui::BeginTabItem(u8"\u6587\u4ef6");
  ImGui::PopStyleColor();
  if (tabFile) {
    ImGui::Spacing();
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);

    
    ImGui::TextColored(ImVec4(0.20f, 0.20f, 0.24f, 1.0f), u8"\u52a8\u4f5c\u6587\u4ef6 (.bin)");
    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    ImGui::InputText("##animpath", g_muscleAnimPath, sizeof(g_muscleAnimPath));
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    if (ImGui::Button(u8"\u6d4f\u89c8##anim", ImVec2(80, 0))) {
      ImGui::PopStyleColor();
      OPENFILENAMEA ofn = {};
      char filePath[512] = "";
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = g_guiHwnd;
      ofn.lpstrFilter = "Muscle Anim (*.bin)\0*.bin\0All Files\0*.*\0";
      ofn.lpstrFile = filePath;
      ofn.nMaxFile = sizeof(filePath);
      ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
      if (GetOpenFileNameA(&ofn)) {
        strncpy(g_muscleAnimPath, filePath, sizeof(g_muscleAnimPath) - 1);
        g_muscleAnimPath[sizeof(g_muscleAnimPath) - 1] = '\0';
        Log("[GUI] Anim selected: %s", g_muscleAnimPath);
        
        if (g_muscleAnim) g_muscleAnim->loaded = false;
        
        
        
        if (g_vmd) { g_vmd = nullptr; g_bsIndicesResolved = false; }
        
        if (g_cameraActive) RestoreCinemachine();
        ResetCameraState();
        if (g_musclePlayer) {
          g_musclePlayer->Stop();
          g_musclePlayer->currentTime = 0;
        }
      }
    } else { ImGui::PopStyleColor(); }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    if (ImGui::Button(u8"\u52a0\u8f7d##anim", ImVec2(-1, 0))) {
      PostMessageW(g_gameHwnd, WM_MMD_GUI_LOAD, 0, 0);
    }
    ImGui::PopStyleColor();

    
    if (g_muscleAnim && g_muscleAnim->loaded) {
      ImGui::TextColored(ImVec4(0.10f, 0.55f, 0.25f, 1.0f),
                         u8"%d \u5e27 / %.1f \u79d2",
                         g_muscleAnim->frameCount, g_muscleAnim->Duration());
    } else {
      ImGui::TextDisabled(u8"\u672a\u52a0\u8f7d");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    
    ImGui::TextColored(ImVec4(0.20f, 0.20f, 0.24f, 1.0f), u8"\u76f8\u673a\u6587\u4ef6 (.vmd)");
    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    ImGui::InputText("##vmdpath", g_cameraVmdPath, sizeof(g_cameraVmdPath));
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    if (ImGui::Button(u8"\u6d4f\u89c8##vmd", ImVec2(80, 0))) {
      ImGui::PopStyleColor();
      OPENFILENAMEA ofn = {};
      char filePath[512] = "";
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = g_guiHwnd;
      ofn.lpstrFilter = "VMD Files (*.vmd)\0*.vmd\0All Files\0*.*\0";
      ofn.lpstrFile = filePath;
      ofn.nMaxFile = sizeof(filePath);
      ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
      if (GetOpenFileNameA(&ofn)) {
        strncpy(g_cameraVmdPath, filePath, sizeof(g_cameraVmdPath) - 1);
        g_cameraVmdPath[sizeof(g_cameraVmdPath) - 1] = '\0';
        Log("[GUI] VMD selected: %s", g_cameraVmdPath);
        
        
        g_cameraVmd = nullptr;
        if (g_cameraActive) RestoreCinemachine();
        ResetCameraState();
        
        if (g_musclePlayer) {
          g_musclePlayer->Stop();
          g_musclePlayer->currentTime = 0;
        }
      }
    } else { ImGui::PopStyleColor(); }
    ImGui::SameLine();
    ImGui::TextDisabled(u8"\u64ad\u653e\u65f6\u81ea\u52a8\u52a0\u8f7d");

    
    if (g_cameraVmd && g_cameraVmd->loaded && !g_cameraVmd->cameraKeys.empty()) {
      ImGui::TextColored(ImVec4(0.10f, 0.55f, 0.25f, 1.0f),
                         u8"%zu \u5173\u952e\u5e27",
                         g_cameraVmd->cameraKeys.size());
    } else {
      ImGui::TextDisabled(u8"\u672a\u52a0\u8f7d");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    
    ImGui::TextColored(ImVec4(0.20f, 0.20f, 0.24f, 1.0f), u8"\u53e3\u578b/\u8868\u60c5 (.vmd)");
    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    ImGui::InputText("##morphpath", g_morphVmdPath, sizeof(g_morphVmdPath));
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    if (ImGui::Button(u8"\u6d4f\u89c8##morph", ImVec2(80, 0))) {
      ImGui::PopStyleColor();
      OPENFILENAMEA ofn = {};
      char filePath[512] = "";
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = g_guiHwnd;
      ofn.lpstrFilter = "VMD Files (*.vmd)\0*.vmd\0All Files\0*.*\0";
      ofn.lpstrFile = filePath;
      ofn.nMaxFile = sizeof(filePath);
      ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
      if (GetOpenFileNameA(&ofn)) {
        strncpy(g_morphVmdPath, filePath, sizeof(g_morphVmdPath) - 1);
        g_morphVmdPath[sizeof(g_morphVmdPath) - 1] = '\0';
        Log("[GUI] Morph VMD selected: %s", g_morphVmdPath);
        
        if (g_vmd) { g_vmd = nullptr; g_bsIndicesResolved = false; }
      }
    } else { ImGui::PopStyleColor(); }
    ImGui::SameLine();
    if (g_morphVmdPath[0] == '\0') {
      ImGui::TextDisabled(u8"\u7a7a = \u81ea\u52a8\u626b\u63cf");
    } else {
      ImGui::TextDisabled(u8"\u64ad\u653e\u65f6\u52a0\u8f7d");
    }

    
    if (g_vmd && g_vmd->loaded && !g_vmd->morphTimelines.empty()) {
      ImGui::TextColored(ImVec4(0.10f, 0.55f, 0.25f, 1.0f),
                         u8"%d \u8868\u60c5\u8f68\u9053",
                         (int)g_vmd->morphTimelines.size());
    } else {
      ImGui::TextDisabled(u8"\u672a\u52a0\u8f7d");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    
    ImGui::TextColored(ImVec4(0.20f, 0.20f, 0.24f, 1.0f), u8"\u97f3\u9891 (.wav/.mp3)");
    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    ImGui::InputText("##audiopath", g_audioPath, sizeof(g_audioPath));
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    if (ImGui::Button(u8"\u6d4f\u89c8##audio", ImVec2(80, 0))) {
      ImGui::PopStyleColor();
      OPENFILENAMEA ofn = {};
      char filePath[512] = "";
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = g_guiHwnd;
      ofn.lpstrFilter = "Audio (*.wav;*.mp3)\0*.wav;*.mp3\0All Files\0*.*\0";
      ofn.lpstrFile = filePath;
      ofn.nMaxFile = sizeof(filePath);
      ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
      if (GetOpenFileNameA(&ofn)) {
        strncpy(g_audioPath, filePath, sizeof(g_audioPath) - 1);
        g_audioPath[sizeof(g_audioPath) - 1] = '\0';
        Log("[GUI] Audio selected: %s", g_audioPath);
        
        PostMessageW(g_gameHwnd, WM_MMD_GUI_LOAD_AUDIO, 0, 0);
      }
    } else { ImGui::PopStyleColor(); }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    if (ImGui::Button(u8"\u52a0\u8f7d##audio", ImVec2(-1, 0))) {
      PostMessageW(g_gameHwnd, WM_MMD_GUI_LOAD_AUDIO, 0, 0);
    }
    ImGui::PopStyleColor();

    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
    ImGui::Checkbox(u8"\u97f3\u9891\u540c\u6b65", &g_audioEnabled);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if (g_audioPath[0] == '\0') {
      ImGui::TextDisabled(u8"\u7a7a = \u9ed8\u8ba4 bgm.wav");
    } else {
      ImGui::TextDisabled(u8"\u53d8\u901f\u65f6\u81ea\u52a8\u5173\u95ed");
    }


    
    if (g_audioPlayer && g_audioPlayer->loaded) {
      ImGui::TextColored(ImVec4(0.10f, 0.55f, 0.25f, 1.0f),
                         u8"\u5df2\u52a0\u8f7d: %.1f \u79d2",
                         g_audioPlayer->GetLengthMs() / 1000.0f);
    } else {
      ImGui::TextDisabled(u8"\u672a\u52a0\u8f7d");
    }

    ImGui::PopStyleVar(); 
    ImGui::EndTabItem();
  }

  ImGui::EndTabBar();

  ImGui::EndChild();
  ImGui::End();
}




static DWORD WINAPI GuiThread(LPVOID) {
  Log("[GUI] Thread started, waiting for game window...");

  
  void *domain = il2cpp_domain_get ? il2cpp_domain_get() : nullptr;
  if (domain && il2cpp_thread_attach)
    il2cpp_thread_attach(domain);

  
  while (g_guiRunning && !g_gameHwnd) {
    Sleep(500);
  }
  if (!g_guiRunning || !g_gameHwnd) return 0;

  Log("[GUI] Game window found: %p", g_gameHwnd);

  
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_CLASSDC;
  wc.lpfnWndProc = GuiWndProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.lpszClassName = L"EIEM_GUI";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  RegisterClassExW(&wc);

  
  RECT gr;
  GetWindowRect(g_gameHwnd, &gr);
  int panelW = 380;
  int panelH = 580;
  int posX = gr.right - panelW - 20;
  int posY = gr.top + 40;

  
  
  
  
  
  
  g_guiHwnd = CreateWindowExW(
      WS_EX_TOOLWINDOW, wc.lpszClassName, L"EIEM",
      WS_POPUP, posX, posY, panelW, panelH,
      nullptr, nullptr, wc.hInstance, nullptr);

  if (!g_guiHwnd) {
    Log("[GUI] ERROR: CreateWindowExW failed! err=%d", GetLastError());
    return 0;
  }

  
  
  
  {
    const DWORD attr = 33; 
    const DWORD pref = 2;  
    DwmSetWindowAttribute(g_guiHwnd, attr, &pref, sizeof(pref));
  }

  
  
  
  
  {
    enum ACCENT_STATE {
      ACCENT_DISABLED = 0,
      ACCENT_ENABLE_GRADIENT = 1,
      ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
      ACCENT_ENABLE_BLURBEHIND = 3,
      ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    };
    struct ACCENT_POLICY {
      DWORD AccentState;
      DWORD AccentFlags;
      DWORD GradientColor; 
      DWORD AnimationId;
    };
    struct WINCOMPATTRDATA {
      DWORD Attrib; 
      PVOID pvData;
      SIZE_T cbData;
    };
    typedef BOOL(WINAPI * pSetWindowCompositionAttribute)(HWND,
                                                          WINCOMPATTRDATA *);

    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    auto SetWCA = (pSetWindowCompositionAttribute)GetProcAddress(
        hUser, "SetWindowCompositionAttribute");
    if (SetWCA) {
      ACCENT_POLICY accent = {};
      accent.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
      accent.AccentFlags = 0;
      
      
      accent.GradientColor = 0x00FFFFFF;
      WINCOMPATTRDATA data = {};
      data.Attrib = 19; 
      data.pvData = &accent;
      data.cbData = sizeof(accent);
      SetWCA(g_guiHwnd, &data);
      Log("[GUI] Acrylic blur-behind enabled");
    } else {
      Log("[GUI] SetWindowCompositionAttribute unavailable");
    }
  }


  
  if (!CreateDeviceD3D(g_guiHwnd)) {
    Log("[GUI] ERROR: CreateDeviceD3D failed!");
    CleanupDeviceD3D();
    DestroyWindow(g_guiHwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
  }

  Log("[GUI] DX11 device created successfully");

  
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr; 
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  
  io.MouseDrawCursor = false;

  
  
  
  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 6.0f;      
  style.FrameRounding = 2.0f;       
  style.GrabRounding = 2.0f;
  style.TabRounding = 0.0f;
  style.WindowBorderSize = 1.0f;    
  style.FrameBorderSize = 1.0f;     
  style.FramePadding = ImVec2(8, 4);
  style.ItemSpacing = ImVec2(8, 6);
  style.WindowPadding = ImVec2(10, 0); 
  style.ScrollbarSize = 12.0f;
  style.ScrollbarRounding = 0.0f;
  style.GrabMinSize = 10.0f;
  style.PopupRounding = 0.0f;

  
  ImVec4 *c = style.Colors;
  
  c[ImGuiCol_WindowBg]       = ImVec4(1.00f, 1.00f, 1.00f, 0.02f); 
  c[ImGuiCol_ChildBg]        = ImVec4(0.12f, 0.12f, 0.12f, 0.00f);
  c[ImGuiCol_PopupBg]        = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
  
  c[ImGuiCol_Border]         = ImVec4(0.55f, 0.55f, 0.58f, 0.40f);
  c[ImGuiCol_BorderShadow]   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  
  c[ImGuiCol_FrameBg]        = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
  c[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
  c[ImGuiCol_FrameBgActive]  = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
  
  c[ImGuiCol_Button]         = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
  c[ImGuiCol_ButtonHovered]  = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
  c[ImGuiCol_ButtonActive]   = ImVec4(0.16f, 0.16f, 0.16f, 1.00f); 
  
  c[ImGuiCol_Header]         = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
  c[ImGuiCol_HeaderHovered]  = ImVec4(0.32f, 0.32f, 0.30f, 1.00f);
  c[ImGuiCol_HeaderActive]   = ImVec4(1.00f, 0.85f, 0.00f, 0.80f); 
  
  c[ImGuiCol_Text]           = ImVec4(0.08f, 0.08f, 0.10f, 1.00f); 
  c[ImGuiCol_TextDisabled]   = ImVec4(0.35f, 0.35f, 0.38f, 1.00f); 
  
  c[ImGuiCol_Separator]      = ImVec4(0.60f, 0.60f, 0.62f, 0.50f);
  c[ImGuiCol_SeparatorHovered]= ImVec4(0.40f, 0.40f, 0.40f, 0.80f);
  c[ImGuiCol_SeparatorActive]= ImVec4(1.00f, 0.85f, 0.00f, 1.00f);
  
  c[ImGuiCol_SliderGrab]     = ImVec4(0.90f, 0.75f, 0.00f, 0.90f);
  c[ImGuiCol_SliderGrabActive]= ImVec4(1.00f, 0.85f, 0.00f, 1.00f);
  
  c[ImGuiCol_CheckMark]      = ImVec4(1.00f, 0.85f, 0.00f, 1.00f);
  
  c[ImGuiCol_PlotHistogram]  = ImVec4(0.85f, 0.70f, 0.00f, 1.00f);
  
  c[ImGuiCol_ScrollbarBg]    = ImVec4(0.10f, 0.10f, 0.10f, 0.40f);
  c[ImGuiCol_ScrollbarGrab]  = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
  c[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
  c[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.85f, 0.00f, 1.00f);
  
  c[ImGuiCol_Tab]            = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
  c[ImGuiCol_TabHovered]     = ImVec4(0.35f, 0.35f, 0.30f, 1.00f);
  c[ImGuiCol_TabActive]      = ImVec4(1.00f, 0.85f, 0.00f, 0.85f);
  
  c[ImGuiCol_ResizeGrip]     = ImVec4(0.30f, 0.30f, 0.30f, 0.40f);
  c[ImGuiCol_ResizeGripHovered]= ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
  c[ImGuiCol_ResizeGripActive]= ImVec4(1.00f, 0.85f, 0.00f, 0.90f);

  
  
  
  {
    ImFontConfig fontCfg;
    fontCfg.OversampleH = 2;
    fontCfg.OversampleV = 1;
    fontCfg.PixelSnapH = true;
    
    const char *fontPath = "C:\\Windows\\Fonts\\msyh.ttc";
    bool loaded = false;
    if (GetFileAttributesA(fontPath) != INVALID_FILE_ATTRIBUTES) {
      ImFont *f = io.Fonts->AddFontFromFileTTF(
          fontPath, 18.0f, &fontCfg,
          io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
      loaded = (f != nullptr);
    }
    if (!loaded) {
      io.Fonts->AddFontDefault();
      Log("[GUI] WARN: msyh.ttc not found, Chinese text may not render");
    }
  }

  
  ImGui_ImplWin32_Init(g_guiHwnd);
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

  Log("[GUI] ImGui initialized, panel ready");



  
  g_guiVisible = false;
  ShowWindow(g_guiHwnd, SW_HIDE);

  
  MSG msg;
  ZeroMemory(&msg, sizeof(msg));
  while (g_guiRunning) {
    
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (msg.message == WM_QUIT) {
        g_guiRunning = false;
        break;
      }
    }
    if (!g_guiRunning) break;

    
    if (!IsWindow(g_gameHwnd) || IsHungAppWindow(g_gameHwnd)) {
      Log("[GUI] Game window gone or hung, shutting down");
      g_guiRunning = false;
      break;
    }

    
    if (!g_guiVisible) {
      Sleep(100); 
      continue;
    }

    
    
    
    
    
    static bool s_panelShown = false;
    HWND fg = GetForegroundWindow();
    bool gameInFocus = false;
    if (fg) {
      if (fg == g_gameHwnd || fg == g_guiHwnd) {
        gameInFocus = true;
      } else {
        
        DWORD fgPid = 0, gamePid = 0;
        GetWindowThreadProcessId(fg, &fgPid);
        if (g_gameHwnd) GetWindowThreadProcessId(g_gameHwnd, &gamePid);
        if (fgPid != 0 && fgPid == gamePid) gameInFocus = true;
      }
    }

    if (gameInFocus && !s_panelShown) {
      SetWindowPos(g_guiHwnd, HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
      ShowWindow(g_guiHwnd, SW_SHOWNOACTIVATE);
      s_panelShown = true;
    } else if (!gameInFocus && s_panelShown) {
      SetWindowPos(g_guiHwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
      ShowWindow(g_guiHwnd, SW_HIDE);
      s_panelShown = false;
    }

    
    if (!s_panelShown) {
      Sleep(80);
      continue;
    }

    
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    
    DrawMainPanel();

    
    ImGui::Render();
    const float clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f}; 
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pMainRenderTargetView,
                                             nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_pMainRenderTargetView,
                                                clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    
    g_pSwapChain->Present(0, 0);
  }

  
  Log("[GUI] Shutting down...");
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  CleanupDeviceD3D();
  DestroyWindow(g_guiHwnd);
  UnregisterClassW(wc.lpszClassName, wc.hInstance);
  g_guiHwnd = nullptr;

  Log("[GUI] Thread exited cleanly");
  return 0;
}




static void ToggleGui() {
  if (!g_guiHwnd) return;
  g_guiVisible = !g_guiVisible;
  if (g_guiVisible) {
    ShowWindow(g_guiHwnd, SW_SHOW);
    SetWindowPos(g_guiHwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetForegroundWindow(g_guiHwnd);
    ReleaseCursorToGui();
  } else {
    SetWindowPos(g_guiHwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ShowWindow(g_guiHwnd, SW_HIDE);
    ReturnCursorToGame();
    if (g_gameHwnd) SetForegroundWindow(g_gameHwnd);
  }
  Log("[GUI] Toggled visibility: %s", g_guiVisible ? "VISIBLE" : "HIDDEN");
}
