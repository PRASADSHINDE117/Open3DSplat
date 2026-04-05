#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "core/Gaussian.h"
#include <glm/glm.hpp>

namespace OpenSplat {
namespace SFM {

// Camera extrinsic and intrinsic properties
struct CameraDetails {
    int id;
    int width, height;
    float focalX, focalY;
    
    // Extrinsics
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    
    std::string imagePath; // Path to original image
};

class DatasetLoader {
public:
    // Automatically scan the directory for the IMGS folder and the models folder (.bin files)
    static bool AutoDetectDataset(const std::string& parentDir, std::string& outImageDir, std::string& outModelDir);

    // Load COLMAP format dataset natively using raw directory paths
    static bool LoadColmapDataset(const std::string& imageDir, const std::string& modelDir, 
                                  std::vector<CameraDetails>& outCameras, 
                                  Core::GaussianScene& outInitialPointCloud);

    // Load from an existing .ply file (e.g. from an already trained model or direct sfm PointCloud)
    static bool LoadPly(const std::string& filepath, Core::GaussianScene& outGaussians);

private:
    static bool ParseCamerasBin(const std::string& path, std::unordered_map<int, CameraDetails>& cameraMap);
    static bool ParseImagesBin(const std::string& path, std::unordered_map<int, CameraDetails>& cameraMap, const std::string& imageDir, std::vector<CameraDetails>& outCameras);
    static bool ParsePoints3DBin(const std::string& path, Core::GaussianScene& pointCloud);
};

} // namespace SFM
} // namespace OpenSplat
