#include "sfm/ColmapWrapper.h"
#include "core/Logger.h"
#include <cstdlib>
#include <string>
#include <cstdio>
#include <regex>

namespace OpenSplat {
namespace SFM {

std::string ColmapWrapper::s_ColmapPath = "colmap"; // Default to PATH

void ColmapWrapper::Init(const std::string& colmapBinPath) {
    if (!colmapBinPath.empty()) {
        s_ColmapPath = colmapBinPath;
    }
}

void ColmapWrapper::RunStage(const std::string& cmd, ColmapProgress& p, std::function<void(const std::string&)> lineParser) {
    OPENSPLAT_CORE_TRACE("Executing: {0}", cmd);
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) {
        p.errorMessage = "Failed to launch colmap subprocess.";
        p.stage = ColmapProgress::Stage::Failed;
        return;
    }
    
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe)) {
        std::string line(buf);
        {
            std::lock_guard<std::mutex> lk(p.logMutex);
            p.lastLogLine = line;
        }
        if (lineParser) lineParser(line);
        if (p.cancelRequested) {
            OPENSPLAT_CORE_WARN("COLMAP stage cancelled.");
            break;
        }
    }
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
}

void ColmapWrapper::RunFeatureExtraction(const std::string& dbPath, const std::string& imageDir, ColmapProgress& p) {
    p.stage = ColmapProgress::Stage::FeatureExtraction;
    std::string cmd = s_ColmapPath + " feature_extractor --database_path " + dbPath + " --image_path " + imageDir;
    
    std::regex rx("Processed file \\[(\\d+)/(\\d+)\\]");
    
    RunStage(cmd, p, [&p, &rx](const std::string& line) {
        std::smatch match;
        if (std::regex_search(line, match, rx) && match.size() == 3) {
            p.imagesProcessed = std::stoi(match[1].str());
            p.totalImages = std::stoi(match[2].str());
        }
    });

    if (!p.cancelRequested && p.stage != ColmapProgress::Stage::Failed) {
        p.imagesProcessed = 0;
        p.totalImages = 0;
    }
}

void ColmapWrapper::RunExhaustiveMatcher(const std::string& dbPath, ColmapProgress& p) {
    p.stage = ColmapProgress::Stage::Matching;
    std::string cmd = s_ColmapPath + " exhaustive_matcher --database_path " + dbPath;
    
    std::regex rx("Matching block \\[(\\d+)/(\\d+)\\]");
    
    RunStage(cmd, p, [&p, &rx](const std::string& line) {
        std::smatch match;
        if (std::regex_search(line, match, rx) && match.size() == 3) {
            p.imagesProcessed = std::stoi(match[1].str());
            p.totalImages = std::stoi(match[2].str());
        }
    });

    if (!p.cancelRequested && p.stage != ColmapProgress::Stage::Failed) {
        p.imagesProcessed = 0;
        p.totalImages = 0;
    }
}

void ColmapWrapper::RunMapper(const std::string& dbPath, const std::string& imageDir, const std::string& outDir, ColmapProgress& p) {
    p.stage = ColmapProgress::Stage::Mapping;
    std::string cmd = s_ColmapPath + " mapper --database_path " + dbPath + " --image_path " + imageDir + " --output_path " + outDir;
    
    std::regex rx("Registering image #(\\d+)");
    
    RunStage(cmd, p, [&p, &rx](const std::string& line) {
        std::smatch match;
        if (std::regex_search(line, match, rx) && match.size() == 2) {
            int unused = std::stoi(match[1].str());
            (void)unused;
        }
        if (line.find("Registering image #") != std::string::npos) {
            p.registeredImages++;
        }
    });
}

} // namespace SFM
} // namespace OpenSplat
