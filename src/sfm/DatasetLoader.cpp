#include "sfm/DatasetLoader.h"
#include "core/Logger.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <glm/gtc/quaternion.hpp>
namespace OpenSplat {
namespace SFM {

// Helper to read primitive types from binary stream
template <typename T>
void ReadBinary(std::ifstream& fs, T& val) {
    fs.read(reinterpret_cast<char*>(&val), sizeof(T));
}

bool DatasetLoader::AutoDetectDataset(const std::string& parentDir, std::string& outImageDir, std::string& outModelDir) {
    outImageDir = "";
    outModelDir = "";
    OPENSPLAT_CORE_INFO("Scanning directory for dataset: {0}", parentDir);

    if (!std::filesystem::exists(parentDir)) return false;

    // Search for Image folder and Model folder recursively (up to 2 levels deep)
    for (const auto& entry : std::filesystem::recursive_directory_iterator(parentDir)) {
        if (entry.is_directory()) {
            std::string path = entry.path().string();
            
            // Check if it has cameras.bin
            if (std::filesystem::exists(path + "/cameras.bin") && std::filesystem::exists(path + "/images.bin")) {
                outModelDir = path;
                OPENSPLAT_CORE_INFO("Auto-detected MODEL directory: {0}", outModelDir);
            }

            // Heuristic for images: folder contains .png or .jpg
            if (outImageDir.empty()) {
                for (const auto& file : std::filesystem::directory_iterator(entry.path())) {
                    if (file.is_regular_file()) {
                        auto ext = file.path().extension().string();
                        // naive check
                        if (ext == ".png" || ext == ".jpg" || ext == ".JPG" || ext == ".PNG") {
                            outImageDir = path;
                            OPENSPLAT_CORE_INFO("Auto-detected IMAGES directory: {0}", outImageDir);
                            break;
                        }
                    }
                }
            }
        }
    }

    return !outImageDir.empty() && !outModelDir.empty();
}

bool DatasetLoader::LoadColmapDataset(const std::string& imageDir, const std::string& modelDir, 
                                      std::vector<CameraDetails>& outCameras, 
                                      Core::GaussianScene& outInitialPointCloud) {
    OPENSPLAT_CORE_INFO("Loading COLMAP dataset. Images: {0}, Models: {1}", imageDir, modelDir);
    
    std::unordered_map<int, CameraDetails> cameraMap;
    
    if (!ParseCamerasBin(modelDir + "/cameras.bin", cameraMap)) return false;
    if (!ParseImagesBin(modelDir + "/images.bin", cameraMap, imageDir, outCameras)) return false;
    if (!ParsePoints3DBin(modelDir + "/points3D.bin", outInitialPointCloud)) return false;
    
    OPENSPLAT_CORE_INFO("Successfully loaded {0} cameras and {1} initial Gaussians.", outCameras.size(), outInitialPointCloud.GetCount());
    return true;
}

bool DatasetLoader::LoadPly(const std::string& filepath, Core::GaussianScene& outGaussians) {
    OPENSPLAT_CORE_INFO("Loading PLY file: {0}", filepath);
    OPENSPLAT_CORE_WARN("PLY parsing not implemented yet.");
    return false;
}

bool DatasetLoader::ParseCamerasBin(const std::string& path, std::unordered_map<int, CameraDetails>& cameraMap) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        OPENSPLAT_CORE_ERROR("Failed to open cameras.bin at {0}", path);
        return false;
    }

    uint64_t num_cameras;
    ReadBinary(file, num_cameras);
    
    for (uint64_t i = 0; i < num_cameras; ++i) {
        int camera_id, model_id;
        uint64_t width, height;
        ReadBinary(file, camera_id);
        ReadBinary(file, model_id);
        ReadBinary(file, width);
        ReadBinary(file, height);

        CameraDetails cam;
        cam.id = camera_id;
        cam.width = (int)width;
        cam.height = (int)height;
        
        // PINHOLE models are usually 1
        if (model_id == 0) {
            double f, cx, cy;
            ReadBinary(file, f); ReadBinary(file, cx); ReadBinary(file, cy);
            cam.focalX = (float)f; cam.focalY = (float)f;
        } else if (model_id == 1) { // PINHOLE
            double fx, fy, cx, cy;
            ReadBinary(file, fx); ReadBinary(file, fy); ReadBinary(file, cx); ReadBinary(file, cy);
            cam.focalX = (float)fx; cam.focalY = (float)fy;
        } else {
            // Simplified skip for other models
            OPENSPLAT_CORE_WARN("Unsupported camera model {0}, treating as PINHOLE by reading 4 params", model_id);
            double dummy[4];
            file.read(reinterpret_cast<char*>(dummy), sizeof(double)*4);
            cam.focalX = (float)dummy[0];
            cam.focalY = (float)dummy[1];
            // Might need advanced skipping depending on model, but this handles basic datasets
        }
        
        cameraMap[camera_id] = cam;
    }
    
    return true;
}

bool DatasetLoader::ParseImagesBin(const std::string& path, std::unordered_map<int, CameraDetails>& cameraMap, const std::string& imageDir, std::vector<CameraDetails>& outCameras) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    uint64_t num_reg_images;
    ReadBinary(file, num_reg_images);

    for (uint64_t i = 0; i < num_reg_images; ++i) {
        int image_id;
        double q[4], t[3];
        int camera_id;
        
        ReadBinary(file, image_id);
        file.read(reinterpret_cast<char*>(q), sizeof(double)*4);
        file.read(reinterpret_cast<char*>(t), sizeof(double)*3);
        ReadBinary(file, camera_id);

        std::string name;
        char c;
        while (file.read(&c, 1) && c != '\0') {
            name += c;
        }

        uint64_t num_points2D;
        ReadBinary(file, num_points2D);
        for (uint64_t j = 0; j < num_points2D; ++j) {
            double x, y; uint64_t p3d_id;
            ReadBinary(file, x); ReadBinary(file, y); ReadBinary(file, p3d_id);
        }

        if (cameraMap.find(camera_id) != cameraMap.end()) {
            CameraDetails cam = cameraMap[camera_id];
            
            // Convert COLMAP quaternion (w, x, y, z) and translation to view matrix
            glm::quat rot((float)q[0], (float)q[1], (float)q[2], (float)q[3]);
            glm::vec3 tra((float)t[0], (float)t[1], (float)t[2]);
            
            glm::mat4 R = glm::mat4_cast(rot);
            cam.viewMatrix = R;
            cam.viewMatrix[3] = glm::vec4(tra, 1.0f);
            
            cam.imagePath = imageDir + "/" + name;
            
            // Replace backslashes just in case
            std::replace(cam.imagePath.begin(), cam.imagePath.end(), '\\', '/');
            
            outCameras.push_back(cam);
        }
    }
    return true;
}

bool DatasetLoader::ParsePoints3DBin(const std::string& path, Core::GaussianScene& pointCloud) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    uint64_t num_points;
    ReadBinary(file, num_points);

    for (uint64_t i = 0; i < num_points; ++i) {
        uint64_t point3D_id;
        double xyz[3];
        uint8_t rgb[3];
        double error;
        
        ReadBinary(file, point3D_id);
        file.read(reinterpret_cast<char*>(xyz), sizeof(double)*3);
        file.read(reinterpret_cast<char*>(rgb), sizeof(uint8_t)*3);
        ReadBinary(file, error);

        uint64_t track_len;
        ReadBinary(file, track_len);
        for (uint64_t j = 0; j < track_len; ++j) {
            int image_id, point2D_idx;
            ReadBinary(file, image_id); ReadBinary(file, point2D_idx);
        }

        Core::Gaussian g;
        g.position = glm::vec3((float)xyz[0], (float)xyz[1], (float)xyz[2]);
        // Base color -> SH coeff 0 conversion heuristic: C * 0.28209
        float r = (rgb[0] / 255.0f);
        float g_c = (rgb[1] / 255.0f);
        float b = (rgb[2] / 255.0f);
        
        g.sh_coeffs[0] = glm::vec3((r - 0.5f) / 0.28209f, (g_c - 0.5f) / 0.28209f, (b - 0.5f) / 0.28209f);
        
        // Initial defaults
        g.scale = glm::vec3(0.01f);
        g.opacity = 0.8f;
        g.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        
        pointCloud.AddGaussian(g);
    }
    
    return true;
}

} // namespace SFM
} // namespace OpenSplat
