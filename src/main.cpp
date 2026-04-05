#include "core/Logger.h"
#include "editor/MainWindow.h"
#include "sfm/DatasetLoader.h"
#include "training/Optimizer.h"
#include "core/Gaussian.h"

int main() {
    // Initialize logging
    OpenSplat::Core::Logger::Init();

    OPENSPLAT_CORE_INFO("Starting OpenSplat C++ Gaussian Splatting Suite...");
    
    // 1. Initialize Window/Editor
    OpenSplat::Editor::MainWindow window(1280, 720, "OpenSplat Editor");
    if (!window.Init()) {
        OPENSPLAT_CORE_FATAL("Failed to initialize Editor Window!");
        return -1;
    }

    // 2. Load dataset (COLMAP sparse model)
    std::vector<OpenSplat::SFM::CameraDetails> cameras;
    OpenSplat::Core::GaussianScene scene;

    // Auto-detect image and model directories inside a default workspace folder
    std::string imgDir, modelDir;
    if (OpenSplat::SFM::DatasetLoader::AutoDetectDataset("workspace", imgDir, modelDir)) {
        OpenSplat::SFM::DatasetLoader::LoadColmapDataset(imgDir, modelDir, cameras, scene);
    } else {
        OPENSPLAT_CORE_WARN("No dataset auto-detected in 'workspace'. Use the UI to configure paths.");
    }

    // 3. Initiate Splat Training Optimizer
    OpenSplat::Training::Optimizer optimizer(scene);

    // Simulate one warm-up training step
    optimizer.Step();

    // 4. Run Editor Event Loop (blocking until window is closed)
    window.Run(scene);

    OPENSPLAT_INFO("Execution complete. Exiting...");

    return 0;
}
