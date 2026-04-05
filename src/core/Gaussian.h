#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace OpenSplat {
namespace Core {

// Represents a single 3D Gaussian Primitive
// Aligned perfectly for GPU buffer transfers
struct Gaussian {
    glm::vec3 position;     // Mean (mu)
    float scalar1;          // Padding for 16-byte alignment (std430, if needed)

    glm::quat rotation;     // Orientation (q)
    
    glm::vec3 scale;        // Scale (s)
    float opacity;          // Alpha (alpha)

    // Spherical Harmonics (SH) coefficients
    // For Degree 0 (1 vec3), Degree 1 (3 vec3s), Degree 2 (5 vec3s), Degree 3 (7 vec3s)
    // Often implementations flatten this array for compute shading 
    // Max degree 3 = 16 coefficients of vec3 (48 floats per gaussian for color)
    glm::vec3 sh_coeffs[16]; 
};

// Represents a collection of Gaussians, managing memory and transfers
class GaussianScene {
public:
    GaussianScene() = default;

    void AddGaussian(const Gaussian& g) {
        m_Gaussians.push_back(g);
    }

    const std::vector<Gaussian>& GetGaussians() const { return m_Gaussians; }
    size_t GetCount() const { return m_Gaussians.size(); }

    void Resize(size_t count) {
        m_Gaussians.resize(count);
    }

    Gaussian* Data() { return m_Gaussians.data(); }

private:
    std::vector<Gaussian> m_Gaussians;
};

} // namespace Core
} // namespace OpenSplat
