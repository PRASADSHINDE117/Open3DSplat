#pragma once

#include "core/Gaussian.h"
#include "sfm/DatasetLoader.h"
#include <vector>

namespace OpenSplat {
namespace Training {

class Optimizer {
public:
    Optimizer(Core::GaussianScene& scene);

    // Run one step of optimization
    // Ingests target images and camera poses, computes loss, and updates Gaussian parameters
    void Step();

    // Data ingestion from DatasetLoader
    void IngestDataset(const std::vector<SFM::CameraDetails>& cameras);

    // Hyperparameters
    void SetLearningRates(float lr_pos, float lr_sh, float lr_opacity, float lr_scale, float lr_rot) {
        m_lr_positions = lr_pos;
        m_lr_features = lr_sh;
        m_lr_opacity = lr_opacity;
        m_lr_scaling = lr_scale;
        m_lr_rotation = lr_rot;
    }
    void SetIterations(int iterations) { m_MaxIterations = iterations; }
    void SetSHDegree(int degree) { m_SHDegree = degree; }

    int GetCurrentIteration() const { return m_CurrentIteration; }

private:
    Core::GaussianScene& m_Scene;
    std::vector<SFM::CameraDetails> m_TrainingCameras;

    // Adam optimizer state vectors (first and second moments)
    std::vector<glm::vec3> m_Moment1_Position;
    std::vector<glm::vec3> m_Moment2_Position;
    
    std::vector<glm::vec3> m_Moment1_Scale;
    std::vector<glm::vec3> m_Moment2_Scale;

    std::vector<glm::quat> m_Moment1_Rotation;
    std::vector<glm::quat> m_Moment2_Rotation;

    std::vector<float> m_Moment1_Opacity;
    std::vector<float> m_Moment2_Opacity;

    // Hyperparameters
    float m_lr_positions = 0.00016f;
    float m_lr_features = 0.0025f;
    float m_lr_opacity = 0.05f;
    float m_lr_scaling = 0.005f;
    float m_lr_rotation = 0.001f;
    
    int m_SHDegree = 3;

    float m_Beta1 = 0.9f;
    float m_Beta2 = 0.999f;
    float m_Epsilon = 1e-8f;
    
    int m_MaxIterations = 30000;
    int m_CurrentIteration = 0;

    // Private Compute Hooks
    void ForwardPass();
    void ComputeLoss();
    void BackwardPass();
    void ApplyAdamStep();
};

} // namespace Training
} // namespace OpenSplat
