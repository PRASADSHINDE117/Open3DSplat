#pragma once

#include <string>
#include "core/Gaussian.h"
#include "core/PointCloudQueue.h"
#include "sfm/ColmapWrapper.h"
#include "editor/Camera.h"
#include "editor/Renderer.h"
#include <mutex>

struct GLFWwindow;

namespace OpenSplat {
namespace Editor {

enum class PanelMode { COLMAP = 0, Training, Editor };

class MainWindow {
public:
    MainWindow(int width, int height, const std::string& title);
    ~MainWindow();

    bool Init();
    void Run(Core::GaussianScene& scene);
    
private:
    int m_Width, m_Height;
    std::string m_Title;
    bool m_IsRunning = false;
    GLFWwindow* m_Window = nullptr;

    // Viewport Rendering
    ArcballCamera m_Camera;
    SceneRenderer m_Renderer;
    GridRenderer m_GridRenderer;

    // Input States
    double m_LastMouseX = 0.0;
    double m_LastMouseY = 0.0;
    bool m_IsMouseDragging = false;
    bool m_IsMiddleDragging = false;

    // Live update synchronization
    Core::PointCloudQueue m_PointQueue;
    SFM::ColmapProgress m_ColmapProgress;

    // Layout
    float m_SidePanelWidth = 400.0f;
    float m_StatusBarHeight = 28.0f;
    PanelMode m_ActivePanel = PanelMode::COLMAP;

    // Rendering
    void ApplyDarkTheme();
    void RenderMenuBar();
    void RenderSidePanel(Core::GaussianScene& scene);
    void RenderViewport();
    void RenderStatusBar(const Core::GaussianScene& scene);
    
    // Panel content
    void RenderColmapPanel(Core::GaussianScene& scene);
    void RenderTrainingPanel(Core::GaussianScene& scene);
    void RenderEditorPanel(Core::GaussianScene& scene);

    // Popups
    bool m_ShowSettings = false;
    bool m_ShowAbout = false;
    void RenderSettingsPopup();

    // File dialogs
    static std::string BrowseForFolder(const std::string& title);

    // COLMAP State
    char m_ColmapWorkspace[512] = "";
    char m_ColmapImageDir[512] = "";
    bool m_ColmapRunning = false;
    int m_ColmapCurrentStep = 0;
    char m_ColmapBinPath[512] = "colmap";
    std::string m_ColmapStatus = "Idle";

    // Training State
    char m_TrainingDatasetPath[512] = "";
    int m_TrainingIterations = 30000;
    float m_LR_Position = 0.00016f;
    float m_LR_Color = 0.0025f;
    float m_LR_Opacity = 0.05f;
    float m_LR_Scaling = 0.005f;
    float m_LR_Rotation = 0.001f;
    int m_SHDegree = 3;
    bool m_IsTraining = false;
    bool m_EnableSHOptimization = true;
    int m_TrainingCurrentIter = 0;

    // Editor State
    float m_BoundingBoxMin[3] = {-5.0f, -5.0f, -5.0f};
    float m_BoundingBoxMax[3] = {5.0f, 5.0f, 5.0f};
    bool m_EnableBoundingBox = false;
    int m_GizmoType = 0;

    // Viewport State
    bool m_ShowGrid = true;
    bool m_ShowAxes = true;
    float m_ZoomSensitivity = 20.0f;

    // FPS
    float m_FPS = 0.0f;
    int m_FrameCount = 0;
    double m_FPSUpdateTime = 0.0;
};

} // namespace Editor
} // namespace OpenSplat
