#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <functional>

namespace OpenSplat {
namespace SFM {

struct ColmapProgress {
    enum class Stage { Idle, FeatureExtraction, Matching, Mapping, Done, Failed };
    std::atomic<Stage> stage{Stage::Idle};
    std::atomic<int>   imagesProcessed{0};
    std::atomic<int>   totalImages{0};
    std::atomic<int>   registeredImages{0};
    
    std::mutex         logMutex;
    std::string        lastLogLine;
    std::string        errorMessage;
    std::atomic<bool>  cancelRequested{false};
};

class ColmapWrapper {
public:
    // Configures the paths to COLMAP binaries (e.g. if not in PATH)
    static void Init(const std::string& colmapBinPath = "");

    // Individual steps with progress reporting
    static void RunFeatureExtraction(const std::string& dbPath, const std::string& imageDir, ColmapProgress& p);
    static void RunExhaustiveMatcher(const std::string& dbPath, ColmapProgress& p);
    static void RunMapper(const std::string& dbPath, const std::string& imageDir, const std::string& outDir, ColmapProgress& p);
    
private:
    static std::string s_ColmapPath;

    // Helper to execute system command and parse stdout live
    static void RunStage(const std::string& cmd, ColmapProgress& p, std::function<void(const std::string&)> lineParser);
};

} // namespace SFM
} // namespace OpenSplat
