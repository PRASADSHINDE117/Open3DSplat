#include "training/Optimizer.h"
#include "core/Logger.h"
#include "cuda/CudaRasterizer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

namespace OpenSplat {
namespace Training {

Optimizer::Optimizer(Core::GaussianScene& scene) 
    : m_Scene(scene) {
    OPENSPLAT_CORE_INFO("Initializing Gaussian Splat Optimizer...");
    
    size_t count = m_Scene.GetCount();
    
    // Initialize Adam moments
    m_Moment1_Position.resize(count, glm::vec3(0.0f));
    m_Moment2_Position.resize(count, glm::vec3(0.0f));
    
    m_Moment1_Scale.resize(count, glm::vec3(0.0f));
    m_Moment2_Scale.resize(count, glm::vec3(0.0f));
    
    m_Moment1_Rotation.resize(count, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    m_Moment2_Rotation.resize(count, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    
    m_Moment1_Opacity.resize(count, 0.0f);
    m_Moment2_Opacity.resize(count, 0.0f);
}

void Optimizer::IngestDataset(const std::vector<SFM::CameraDetails>& cameras) {
    OPENSPLAT_CORE_INFO("Optimizer ingesting {0} cameras for training.", cameras.size());
    m_TrainingCameras = cameras;

    for (const auto& cam : m_TrainingCameras) {
        if (!cam.imagePath.empty()) {
            int width, height, channels;
            // Load image using stb_image
            unsigned char* data = stbi_load(cam.imagePath.c_str(), &width, &height, &channels, 4); // force RGBA
            if (data) {
                // In a real implementation, copy this data to a GPU Texture buffer
                // e.g. CUDA::CopyImageToDevice(data, width, height);
                OPENSPLAT_CORE_TRACE("Loaded training image: {0} ({1}x{2})", cam.imagePath, width, height);
                stbi_image_free(data);
            } else {
                OPENSPLAT_CORE_WARN("Failed to load training image: {0}", cam.imagePath);
            }
        }
    }
}

void Optimizer::Step() {
    if (m_CurrentIteration >= m_MaxIterations) {
        return;
    }

    ForwardPass();
    ComputeLoss();
    BackwardPass();
    ApplyAdamStep();

    m_CurrentIteration++;
    
    if (m_CurrentIteration % 100 == 0) {
        OPENSPLAT_CORE_TRACE("Training Step {0}/{1} Completed.", m_CurrentIteration, m_MaxIterations);
    }
}

void Optimizer::ForwardPass() {
    OPENSPLAT_CORE_TRACE("Optimizer::ForwardPass()");
    // Call the CUDA Forward Pass
    // In reality, we extract pointers from m_Scene
    CUDA::CudaRasterizer::Forward(m_Scene.GetCount(),
        1280, 720, nullptr, nullptr, nullptr, nullptr, 
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

void Optimizer::ComputeLoss() {
    OPENSPLAT_CORE_TRACE("Optimizer::ComputeLoss()");
    // Compare rasterized image to ground truth COLMAP image
    // Compute L1 loss and D-SSIM loss
}

void Optimizer::BackwardPass() {
    OPENSPLAT_CORE_TRACE("Optimizer::BackwardPass()");
    // Call the CUDA Backward Pass
    CUDA::CudaRasterizer::Backward(m_Scene.GetCount(),
        1280, 720, nullptr, nullptr, nullptr, nullptr, 
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

void Optimizer::ApplyAdamStep() {
    OPENSPLAT_CORE_TRACE("Optimizer::ApplyAdamStep() with LR pos: {0}, color: {1}", m_lr_positions, m_lr_features);
    
    // Simulate Adam per-parameter update logic
    // Using parameters: m_Beta1, m_Beta2, m_Epsilon
    const size_t count = m_Scene.GetCount();
    for (size_t i = 0; i < count; ++i) {
        // e.g., Update position
        // glm::vec3 pos_grad = GetGradientPosition(i);
        // m_Moment1_Position[i] = m_Beta1 * m_Moment1_Position[i] + (1.0f - m_Beta1) * pos_grad;
        // m_Moment2_Position[i] = m_Beta2 * m_Moment2_Position[i] + (1.0f - m_Beta2) * (pos_grad * pos_grad);
        //
        // glm::vec3 m_hat = m_Moment1_Position[i] / (1.0f - pow(m_Beta1, m_CurrentIteration + 1));
        // glm::vec3 v_hat = m_Moment2_Position[i] / (1.0f - pow(m_Beta2, m_CurrentIteration + 1));
        //
        // m_Scene.GetGaussian(i).position -= m_lr_positions * m_hat / (sqrt(v_hat) + m_Epsilon);
        
        // Similarly apply for scale, rotation, opacity, and features (SH)
        // using m_lr_scaling, m_lr_rotation, m_lr_opacity, and m_lr_features
    }

    // Also handle densification / pruning logic here periodically:
    // if (m_CurrentIteration % 100 == 0 && m_CurrentIteration < m_MaxIterations / 2) {
    //      DensifyAndPrune(...);
    // }
}

} // namespace Training
} // namespace OpenSplat
