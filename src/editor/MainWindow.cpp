#include "editor/MainWindow.h"
#include "core/Logger.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h> 
#endif

#include <thread>
#include <filesystem>

#include "sfm/ColmapWrapper.h"
#include "sfm/DatasetLoader.h"
#include "training/Optimizer.h"

namespace OpenSplat {
namespace Editor {

MainWindow::MainWindow(int width, int height, const std::string& title)
    : m_Width(width), m_Height(height), m_Title(title) {
}

MainWindow::~MainWindow() {
    OPENSPLAT_CORE_INFO("Shutting down Editor UI...");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (m_Window) {
        glfwDestroyWindow(m_Window);
    }
    glfwTerminate();
}

bool MainWindow::Init() {
    OPENSPLAT_CORE_INFO("Initializing window: {0} ({1}x{2})", m_Title, m_Width, m_Height);
    
    if (!glfwInit()) {
        OPENSPLAT_CORE_ERROR("Failed to initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        OPENSPLAT_CORE_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(1); // Enable vsync

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    // Enable docking if available (optional, but good for complex layouts)
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 

    ApplyDarkTheme();

    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize Main OpenGL Renderer
    if (!m_Renderer.Init()) {
        OPENSPLAT_CORE_ERROR("Failed to initialize SceneRenderer");
    }
    
    // Initialize Grid/Gizmo Renderer
    if (!m_GridRenderer.Init()) {
        OPENSPLAT_CORE_ERROR("Failed to initialize GridRenderer");
        return false;
    }

    m_IsRunning = true;
    return true;
}

void MainWindow::ApplyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.14f, 0.14f, 0.14f, 0.94f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.40f, 0.90f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.40f, 0.90f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.50f, 0.98f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
    
    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 3.0f;
    style.GrabRounding      = 3.0f;
    style.ScrollbarRounding = 3.0f;
}

std::string MainWindow::BrowseForFolder(const std::string& title) {
#ifdef _WIN32
    std::string result = "";
    IFileOpenDialog* pDialog;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pDialog));
    if (SUCCEEDED(hr)) {
        DWORD dwOptions;
        if (SUCCEEDED(pDialog->GetOptions(&dwOptions))) {
            pDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }
        if (!title.empty()) {
            int wlen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, NULL, 0);
            std::wstring wtitle(wlen, 0);
            MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wtitle[0], wlen);
            pDialog->SetTitle(wtitle.c_str());
        }
        hr = pDialog->Show(NULL);
        if (SUCCEEDED(hr)) {
            IShellItem* pItem;
            hr = pDialog->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr)) {
                    int len = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    result.assign(len - 1, 0);
                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &result[0], len, NULL, NULL);
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pDialog->Release();
    }
    return result;
#else
    // Fallback for non-Windows (could use zenity or kdialog)
    return "";
#endif
}

void MainWindow::Run(Core::GaussianScene& scene) {
    OPENSPLAT_CORE_INFO("Starting main editor render loop...");
    
#ifdef _WIN32
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
#endif

    // Initial GPU buffer push if scene already has data
    if (scene.GetCount() > 0) {
        m_Renderer.UpdateSceneData(scene);
    }

    while (!glfwWindowShouldClose(m_Window)) {
        glfwPollEvents();

        // Poll lock-free queue each frame
        std::vector<Core::Gaussian> pts;
        if (m_PointQueue.TryPop(pts)) {
            m_Renderer.UpdateSceneData(pts);
        }

 
        
        int display_w, display_h;
        glfwGetFramebufferSize(m_Window, &display_w, &display_h);
        float menuBarHeight = ImGui::GetFrameHeight();
        
        // Update camera matrices bounds based on the active 3D viewport region
        int viewX = (int)m_SidePanelWidth;
        int viewY = (int)m_StatusBarHeight;
        int viewW = display_w - viewX;
        int viewH = display_h - (int)menuBarHeight - viewY;
        
        m_Camera.Update(viewW, viewH);

        // Calculate FPS
        m_FrameCount++;
        double currentTime = glfwGetTime();
        if (currentTime - m_FPSUpdateTime >= 1.0) {
            m_FPS = (float)m_FrameCount / (float)(currentTime - m_FPSUpdateTime);
            m_FrameCount = 0;
            m_FPSUpdateTime = currentTime;
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // --- Handle Camera Input ---
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) { // Only process if ImGui is not actively using the mouse
            double mX, mY;
            glfwGetCursorPos(m_Window, &mX, &mY);
            
            bool leftDown = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            bool midDown = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
            bool rightDown = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
            bool shiftDown = glfwGetKey(m_Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(m_Window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
            
            double dx = mX - m_LastMouseX;
            double dy = mY - m_LastMouseY;
            
            if (midDown) {
                if (m_IsMouseDragging) {
                    if (!shiftDown) { // Orbit
                        m_Camera.Yaw(dx * -0.01f);
                        m_Camera.Pitch(dy * -0.01f);
                    } else { // Pan
                        m_Camera.Pan(dx * 1.5f, dy * 1.5f);
                    }
                }
                m_IsMouseDragging = true;
            } else {
                m_IsMouseDragging = false;
            }
            
            bool fDown = glfwGetKey(m_Window, GLFW_KEY_F) == GLFW_PRESS;
            static bool lastFDown = false;
            if (fDown && !lastFDown) {
                const auto& gaussians = scene.GetGaussians();
                if (!gaussians.empty()) {
                    glm::vec3 minBound(1e10f);
                    glm::vec3 maxBound(-1e10f);
                    for (const auto& g : gaussians) {
                        minBound = glm::min(minBound, g.position);
                        maxBound = glm::max(maxBound, g.position);
                    }
                    glm::vec3 center = (minBound + maxBound) * 0.5f;
                    float radius = glm::length(maxBound - minBound) * 0.5f;
                    m_Camera.SetTarget(center);
                    m_Camera.SetDistance(radius + 5.0f);
                }
            }
            lastFDown = fDown;
            
            m_LastMouseX = mX;
            m_LastMouseY = mY;
            
            float scrollY = io.MouseWheel;
            if (scrollY != 0.0f) {
                m_Camera.Zoom(scrollY * m_ZoomSensitivity);
            }
        }

        RenderMenuBar();
        RenderSidePanel(scene);
        RenderViewport();
        RenderStatusBar(scene);
        RenderSettingsPopup();

        // Render OpenGL Viewport background
        glViewport(0, 0, display_w, display_h);
        
        // Full screen clear for UI garbage prevention
        glClearColor(0.04f, 0.04f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Define scissor box for the 3D viewport (leaving space for UI)
        glEnable(GL_SCISSOR_TEST);
        glScissor(viewX, viewY, viewW, viewH);
        
        // Dark grid background
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        
        glm::mat4 vp = m_Camera.GetViewProjMatrix();
        m_GridRenderer.Render(vp);
        m_Renderer.Render(vp, viewW, viewH);
        
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);

        // End ImGui frame
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_Window);
    }
    
#ifdef _WIN32
    CoUninitialize();
#endif
}

void MainWindow::RenderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import PLY...")) {}
            if (ImGui::MenuItem("Export SPZ...")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(m_Window, true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Preferences")) {
                m_ShowSettings = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Grid", nullptr, &m_ShowGrid);
            ImGui::MenuItem("Show Axes", nullptr, &m_ShowAxes);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Prune Gaussians")) {}
            if (ImGui::MenuItem("Densify Gaussians")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About OpenSplat")) { m_ShowAbout = true; }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    if (m_ShowAbout) {
        ImGui::OpenPopup("About OpenSplat");
        if (ImGui::BeginPopupModal("About OpenSplat", &m_ShowAbout, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("OpenSplat Engine v0.1.0");
            ImGui::Separator();
            ImGui::Text("A high-performance C++ Gaussian Splatting suite.");
            ImGui::Text("Inspired by Licht Studio and original Inria implementation.");
            ImGui::Spacing();
            if (ImGui::Button("Close", ImVec2(120, 0))) { m_ShowAbout = false; }
            ImGui::EndPopup();
        }
    }
}

void MainWindow::RenderSettingsPopup() {
    if (m_ShowSettings) {
        ImGui::OpenPopup("Preferences");
    }
    if (ImGui::BeginPopupModal("Preferences", &m_ShowSettings, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("External Tools");
        ImGui::Separator();
        
        ImGui::Text("COLMAP Executable Path:");
        ImGui::InputText("##colmapbin", m_ColmapBinPath, IM_ARRAYSIZE(m_ColmapBinPath));
        ImGui::SameLine();
        if (ImGui::Button("Browse##colmapbin")) {
            // Need a file picker here, but for now we have a folder picker
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        
        if (ImGui::Button("Save Configuration", ImVec2(120, 0))) {
            OpenSplat::SFM::ColmapWrapper::Init(std::string(m_ColmapBinPath));
            m_ShowSettings = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            m_ShowSettings = false;
        }
        ImGui::EndPopup();
    }
}

void MainWindow::RenderSidePanel(Core::GaussianScene& scene) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float menuBarHeight = ImGui::GetFrameHeight();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + menuBarHeight));
    ImGui::SetNextWindowSize(ImVec2(m_SidePanelWidth, viewport->WorkSize.y - menuBarHeight - m_StatusBarHeight));
    
    ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | 
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                  ImGuiWindowFlags_NoBringToFrontOnFocus;
                                  
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    ImGui::Begin("SidePanel", nullptr, panelFlags);
    
    // Custom segmented control for tabs (looks more modern than basic tabs)
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 8));
    
    float btnWidth = (ImGui::GetContentRegionAvail().x - 8) / 3.0f;
    
    ImVec4 activeCol = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
    ImVec4 inactiveCol = ImGui::GetStyle().Colors[ImGuiCol_Button];
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_ActivePanel == PanelMode::COLMAP ? activeCol : inactiveCol);
    if (ImGui::Button("COLMAP", ImVec2(btnWidth, 24))) m_ActivePanel = PanelMode::COLMAP;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_ActivePanel == PanelMode::Training ? activeCol : inactiveCol);
    if (ImGui::Button("Train", ImVec2(btnWidth, 24))) m_ActivePanel = PanelMode::Training;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_ActivePanel == PanelMode::Editor ? activeCol : inactiveCol);
    if (ImGui::Button("Editor", ImVec2(btnWidth, 24))) m_ActivePanel = PanelMode::Editor;
    ImGui::PopStyleColor();
    
    ImGui::PopStyleVar(2);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Render active panel content
    ImGui::BeginChild("PanelContent", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
    
    switch (m_ActivePanel) {
        case PanelMode::COLMAP:   RenderColmapPanel(scene); break;
        case PanelMode::Training: RenderTrainingPanel(scene); break;
        case PanelMode::Editor:   RenderEditorPanel(scene); break;
    }
    
    ImGui::EndChild();
    
    ImGui::End(); // SidePanel
    ImGui::PopStyleVar();
}

void MainWindow::RenderViewport() {
    // We render the 3D viewport using OpenGL scissor test in Run().
    // We could also do render-to-texture and show it as an ImGui Image, but direct rendering is faster.
    
    // We can put floating toolbars here overlaying the viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float menuBarHeight = ImGui::GetFrameHeight();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + m_SidePanelWidth + 10, viewport->WorkPos.y + menuBarHeight + 10));
    ImGui::SetNextWindowBgAlpha(0.6f);
    
    ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
                                    
    if (ImGui::Begin("ViewportOverlay", nullptr, overlayFlags)) {
        std::string modeStr = "View Mode: ";
        modeStr += (m_ActivePanel == PanelMode::Editor) ? "Splat Editor" : "Camera Orbit";
        ImGui::Text("%s", modeStr.c_str());
        
        if (m_ActivePanel == PanelMode::Editor) {
            ImGui::Separator();
            ImGui::RadioButton("Translate", &m_GizmoType, 0); ImGui::SameLine();
            ImGui::RadioButton("Rotate", &m_GizmoType, 1); ImGui::SameLine();
            ImGui::RadioButton("Scale", &m_GizmoType, 2);
        }
    }
    ImGui::End();

    // Top Right Overlay for Viewport Settings
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 240, viewport->WorkPos.y + menuBarHeight + 10));
    ImGui::SetNextWindowBgAlpha(0.6f);
    if (ImGui::Begin("SettingsOverlay", nullptr, overlayFlags)) {
        ImGui::Text("Camera Settings");
        ImGui::Separator();
        ImGui::SetNextItemWidth(120);
        ImGui::SliderFloat("Zoom Sens", &m_ZoomSensitivity, 1.0f, 100.0f, "%.1f");
    }
    ImGui::End();
}

void MainWindow::RenderStatusBar(const Core::GaussianScene& scene) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - m_StatusBarHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, m_StatusBarHeight));
    
    ImGuiWindowFlags statusFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | 
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
                                   
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 4));
    ImGui::Begin("StatusBar", nullptr, statusFlags);
    
    ImGui::Text("FPS: %.1f", m_FPS);
    ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
    
    ImGui::Text("Gaussians: %zu", scene.GetCount());
    ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
    
    std::string statusMsg = "Ready";
    if (m_ColmapRunning) statusMsg = "COLMAP: " + m_ColmapStatus;
    else if (m_IsTraining) statusMsg = "Training: Iteration " + std::to_string(m_TrainingCurrentIter);
    
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Status: %s", statusMsg.c_str());
    
    ImGui::End();
    ImGui::PopStyleVar();
}

void MainWindow::RenderColmapPanel(Core::GaussianScene& scene) {
    ImGui::TextDisabled("STRUCTURE FROM MOTION");
    ImGui::Spacing();
    
    // Inputs
    ImGui::Text("Images Directory:");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40);
    ImGui::InputText("##imgdir", m_ColmapImageDir, IM_ARRAYSIZE(m_ColmapImageDir));
    ImGui::SameLine();
    if (ImGui::Button("...##img", ImVec2(32, 0))) {
        std::string path = BrowseForFolder("Select Images Directory");
        if (!path.empty()) strncpy(m_ColmapImageDir, path.c_str(), sizeof(m_ColmapImageDir)-1);
    }
    
    ImGui::Spacing();
    
    ImGui::Text("Workspace Directory:");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40);
    ImGui::InputText("##workdir", m_ColmapWorkspace, IM_ARRAYSIZE(m_ColmapWorkspace));
    ImGui::SameLine();
    if (ImGui::Button("...##work", ImVec2(32, 0))) {
        std::string path = BrowseForFolder("Select Workspace Directory");
        if (!path.empty()) strncpy(m_ColmapWorkspace, path.c_str(), sizeof(m_ColmapWorkspace)-1);
    }
    
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    
    auto stage = m_ColmapProgress.stage.load();
    if (stage != OpenSplat::SFM::ColmapProgress::Stage::Idle && stage != OpenSplat::SFM::ColmapProgress::Stage::Done && stage != OpenSplat::SFM::ColmapProgress::Stage::Failed) {
        ImGui::Text("Pipeline Running...");
        ImGui::Spacing();
        
        // Stage 1
        float s1_prog = 0.0f;
        std::string s1_text = "Waiting...";
        if (stage == OpenSplat::SFM::ColmapProgress::Stage::FeatureExtraction) {
            int p = m_ColmapProgress.imagesProcessed;
            int t = m_ColmapProgress.totalImages;
            if (t > 0) s1_prog = (float)p / t;
            s1_text = std::to_string(p) + " / " + std::to_string(t) + " images";
        } else if (stage > OpenSplat::SFM::ColmapProgress::Stage::FeatureExtraction) {
            s1_prog = 1.0f; s1_text = "Done";
        }
        ImGui::Text("[Stage 1: Feature Extraction]"); ImGui::SameLine(250);
        ImGui::ProgressBar(s1_prog, ImVec2(-1, 0), s1_text.c_str());

        // Stage 2
        float s2_prog = 0.0f;
        std::string s2_text = "Waiting...";
        if (stage == OpenSplat::SFM::ColmapProgress::Stage::Matching) {
            int p = m_ColmapProgress.imagesProcessed;
            int t = m_ColmapProgress.totalImages;
            if (t > 0) s2_prog = (float)p / t;
            s2_text = std::to_string(p) + " / " + std::to_string(t) + " blocks";
        } else if (stage > OpenSplat::SFM::ColmapProgress::Stage::Matching) {
            s2_prog = 1.0f; s2_text = "Done";
        }
        ImGui::Text("[Stage 2: Feature Matching  ]"); ImGui::SameLine(250);
        ImGui::ProgressBar(s2_prog, ImVec2(-1, 0), s2_text.c_str());

        // Stage 3
        float s3_prog = 0.0f;
        std::string s3_text = "Waiting...";
        if (stage == OpenSplat::SFM::ColmapProgress::Stage::Mapping) {
            s3_prog = -1.0f * (float)ImGui::GetTime(); // Indeterminate until we know total
            s3_text = "Registering...";
        } else if (stage > OpenSplat::SFM::ColmapProgress::Stage::Mapping) {
            s3_prog = 1.0f; s3_text = "Done";
        }
        ImGui::Text("[Stage 3: Sparse Mapping    ]"); ImGui::SameLine(250);
        ImGui::ProgressBar(s3_prog, ImVec2(-1, 0), s3_text.c_str());

        ImGui::Spacing();
        ImGui::Text("Registered images: %d        Point cloud: %zu pts", m_ColmapProgress.registeredImages.load(), m_Renderer.GetUploadedCount());
        
        ImGui::Spacing();
        std::string lastLog;
        {
            std::lock_guard<std::mutex> lk(m_ColmapProgress.logMutex);
            lastLog = m_ColmapProgress.lastLogLine;
        }
        ImGui::TextDisabled("Log: %s", lastLog.c_str());
        
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("CANCEL", ImVec2(-1, 40))) {
            m_ColmapProgress.cancelRequested = true;
        }
        ImGui::PopStyleColor();
    } else {
        if (stage == OpenSplat::SFM::ColmapProgress::Stage::Failed) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", m_ColmapProgress.errorMessage.c_str());
            ImGui::Spacing();
        } else if (stage == OpenSplat::SFM::ColmapProgress::Stage::Done) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Completed Successfully!");
            ImGui::Spacing();
        }

        if (ImGui::Button("START AUTOMATED PIPELINE", ImVec2(-1, 40))) {
            std::string imgDir = m_ColmapImageDir;
            std::string workDir = m_ColmapWorkspace;
            if (!imgDir.empty() && !workDir.empty()) {
                m_ColmapProgress.cancelRequested = false;
                m_ColmapProgress.errorMessage = "";
                m_ColmapProgress.registeredImages = 0;
                
                // Spawn combined background thread
                std::thread([this, imgDir, workDir]() {
                    std::filesystem::create_directories(workDir + "/database");
                    std::string dbPath = workDir + "/database.db";
                    
                    OpenSplat::SFM::ColmapWrapper::RunFeatureExtraction(dbPath, imgDir, m_ColmapProgress);
                    if (!m_ColmapProgress.cancelRequested && m_ColmapProgress.stage != OpenSplat::SFM::ColmapProgress::Stage::Failed) {
                        OpenSplat::SFM::ColmapWrapper::RunExhaustiveMatcher(dbPath, m_ColmapProgress);
                    }
                    
                    if (!m_ColmapProgress.cancelRequested && m_ColmapProgress.stage != OpenSplat::SFM::ColmapProgress::Stage::Failed) {
                        std::filesystem::create_directories(workDir + "/sparse/0");
                        
                        // File watcher thread for mapping
                        std::atomic<bool> watching{true};
                        std::thread watcher([this, workDir, imgDir, &watching]() {
                            std::filesystem::file_time_type lastLoadTime;
                            std::string modelPath = workDir + "/sparse/0/points3D.bin";
                            while (watching) {
                                std::this_thread::sleep_for(std::chrono::seconds(1));
                                if (std::filesystem::exists(modelPath)) {
                                    auto currentModTime = std::filesystem::last_write_time(modelPath);
                                    auto fileSize = std::filesystem::file_size(modelPath);
                                    if (currentModTime != lastLoadTime && fileSize > 0) {
                                        OpenSplat::Core::GaussianScene tempScene;
                                        std::vector<OpenSplat::SFM::CameraDetails> dCams;
                                        if (OpenSplat::SFM::DatasetLoader::LoadColmapDataset(imgDir, workDir + "/sparse/0", dCams, tempScene)) {
                                            if (tempScene.GetCount() > 0) {
                                                // Extract to vector explicitly to push to lock-free queue
                                                std::vector<OpenSplat::Core::Gaussian> pts(tempScene.Data(), tempScene.Data() + tempScene.GetCount());
                                                m_PointQueue.TryPush(std::move(pts));
                                            }
                                        }
                                        lastLoadTime = currentModTime;
                                    }
                                }
                            }
                        });
                        
                        OpenSplat::SFM::ColmapWrapper::RunMapper(dbPath, imgDir, workDir + "/sparse/0", m_ColmapProgress);
                        watching = false;
                        watcher.join();
                    }
                    
                    if (!m_ColmapProgress.cancelRequested && m_ColmapProgress.stage != OpenSplat::SFM::ColmapProgress::Stage::Failed) {
                        m_ColmapProgress.stage = OpenSplat::SFM::ColmapProgress::Stage::Done;
                    }
                }).detach();
            }
        }
    }
}

void MainWindow::RenderTrainingPanel(Core::GaussianScene& scene) {
    ImGui::TextDisabled("GAUSSIAN SPLAT OPTIMIZER");
    ImGui::Spacing();
    
    ImGui::Text("Dataset Source (COLMAP Output):");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40);
    ImGui::InputText("##datasetdir", m_TrainingDatasetPath, IM_ARRAYSIZE(m_TrainingDatasetPath));
    ImGui::SameLine();
    if (ImGui::Button("...##ds", ImVec2(32, 0))) {
        std::string path = BrowseForFolder("Select COLMAP Dataset Directory");
        if (!path.empty()) strncpy(m_TrainingDatasetPath, path.c_str(), sizeof(m_TrainingDatasetPath)-1);
    }
    
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    
    if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputInt("Max Iterations", &m_TrainingIterations, 100, 1000);
        ImGui::SliderInt("SH Degree", &m_SHDegree, 0, 3);
        ImGui::Checkbox("Optimize Spherical Harmonics", &m_EnableSHOptimization);
        
        if (ImGui::TreeNode("Learning Rates")) {
            ImGui::DragFloat("Position", &m_LR_Position, 0.00001f, 0.0f, 0.001f, "%.6f");
            ImGui::DragFloat("Color", &m_LR_Color, 0.0001f, 0.0f, 0.01f, "%.5f");
            ImGui::DragFloat("Scale", &m_LR_Scaling, 0.0001f, 0.0f, 0.01f, "%.5f");
            ImGui::DragFloat("Rotation", &m_LR_Rotation, 0.0001f, 0.0f, 0.01f, "%.5f");
            ImGui::DragFloat("Opacity", &m_LR_Opacity, 0.001f, 0.0f, 0.1f, "%.4f");
            ImGui::TreePop();
        }
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (m_IsTraining) {
        float progress = (float)m_TrainingCurrentIter / (float)m_TrainingIterations;
        ImGui::ProgressBar(progress, ImVec2(-1, 0));
        ImGui::Text("Iteration: %d / %d", m_TrainingCurrentIter, m_TrainingIterations);
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        if (ImGui::Button("PAUSE TRAINING", ImVec2(-1, 40))) {
            m_IsTraining = false;
        }
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        if (ImGui::Button("START TRAINING", ImVec2(-1, 40))) {
            std::string dsPath = m_TrainingDatasetPath;
            if (!dsPath.empty()) {
                m_IsTraining = true;
                std::thread([this, dsPath, &scene]() {
                    std::string imgDir, modelDir;
                    if (OpenSplat::SFM::DatasetLoader::AutoDetectDataset(dsPath, imgDir, modelDir)) {
                        std::vector<OpenSplat::SFM::CameraDetails> cameras;
                        if (OpenSplat::SFM::DatasetLoader::LoadColmapDataset(imgDir, modelDir, cameras, scene)) {
                            OpenSplat::Training::Optimizer optimizer(scene);
                            optimizer.SetLearningRates(m_LR_Position, m_LR_Color, m_LR_Opacity, m_LR_Scaling, m_LR_Rotation);
                            optimizer.SetIterations(m_TrainingIterations);
                            optimizer.SetSHDegree(m_SHDegree);
                            optimizer.IngestDataset(cameras);
                            
                            // To resume, we'd need to pass m_TrainingCurrentIter into the optimizer.
                            // For now we just loop and update the UI counter.
                            while (m_IsTraining && m_TrainingCurrentIter < m_TrainingIterations) {
                                optimizer.Step();
                                m_TrainingCurrentIter = optimizer.GetCurrentIteration();
                            }
                        }
                    }
                    m_IsTraining = false;
                }).detach();
            }
        }
        ImGui::PopStyleColor(2);
    }
}

void MainWindow::RenderEditorPanel(Core::GaussianScene& scene) {
    ImGui::TextDisabled("SCENE GRAPH");
    ImGui::Spacing();
    
    if (ImGui::TreeNodeEx("Root", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        if (scene.GetCount() > 0) {
            ImGui::TreeNodeEx("Gaussian Cloud", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet);
            ImGui::SameLine(); ImGui::TextDisabled("(%zu pts)", scene.GetCount());
        } else {
            ImGui::TextDisabled("No data loaded.");
        }
        ImGui::TreePop();
    }
    
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    
    ImGui::TextDisabled("MODIFIERS");
    ImGui::Spacing();
    
    if (ImGui::CollapsingHeader("Volume Culling", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable AABB Filter", &m_EnableBoundingBox);
        if (m_EnableBoundingBox) {
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 40);
            ImGui::DragFloat3("Min", m_BoundingBoxMin, 0.1f);
            ImGui::DragFloat3("Max", m_BoundingBoxMax, 0.1f);
            ImGui::PopItemWidth();
            
            if (ImGui::Button("Apply Hard Cull", ImVec2(-1, 0))) {
                // Future: Implement actual subset filtering in GaussianScene
                OPENSPLAT_CORE_INFO("Culling applied.");
            }
        }
    }
    
    if (ImGui::CollapsingHeader("Density Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Prune low-opacity filters < 5%", ImVec2(-1, 0))) {}
        if (ImGui::Button("Force Densify Large Splats", ImVec2(-1, 0))) {}
    }
}

} // namespace Editor
} // namespace OpenSplat
