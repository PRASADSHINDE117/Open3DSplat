#pragma once

#include "core/Gaussian.h"
#include <glm/glm.hpp>
#include <vector>

namespace OpenSplat {
namespace Editor {

class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();

    // Init shaders and buffers (Must be called with active GL context)
    bool Init();
    
    // Upload points to VBO
    void UpdateSceneData(Core::GaussianScene& scene);
    void UpdateSceneData(const std::vector<Core::Gaussian>& points);

    // Draw the scene using the provided camera ViewProjection matrix
    void Render(const glm::mat4& viewProj, int width, int height);
    
    void SetPointSize(float size) { m_PointSize = size; }
    size_t GetUploadedCount() const { return m_UploadedPointCount; }

private:
    unsigned int m_ShaderProgram = 0;
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    size_t m_UploadedPointCount = 0;
    float m_PointSize = 2.0f;

    unsigned int CompileShader(unsigned int type, const char* source);
    bool CreateShaderProgram();
};

class GridRenderer {
public:
    GridRenderer();
    ~GridRenderer();
    
    bool Init();
    void Render(const glm::mat4& viewProj);

private:
    unsigned int m_ShaderProgram = 0;
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    size_t m_VertexCount = 0;
    
    unsigned int CompileShader(unsigned int type, const char* source);
    bool CreateShaderProgram();
};

} // namespace Editor
} // namespace OpenSplat
