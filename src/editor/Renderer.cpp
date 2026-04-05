#include "editor/Renderer.h"
#include "core/Logger.h"
#include <backends/imgui_impl_opengl3_loader.h>
#include <GLFW/glfw3.h>

#ifndef GL_POINTS
#define GL_POINTS 0x0000
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE 0x8642
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
typedef void (*openSplat_PFNGLDRAWARRAYSPROC)(unsigned int mode, int first, int count);
typedef void (*openSplat_PFNGLUNIFORM1FPROC)(int location, float v0);

static openSplat_PFNGLDRAWARRAYSPROC openSplat_glDrawArrays = nullptr;
static openSplat_PFNGLUNIFORM1FPROC openSplat_glUniform1f = nullptr;

namespace OpenSplat {
namespace Editor {

static const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColorBase; // Expecting SH0 or basic RGB

uniform mat4 u_ViewProj;
uniform float u_PointSize;

out float vDepth;
out vec3 vColor;

void main()
{
    gl_Position = u_ViewProj * vec4(aPos, 1.0);
    gl_PointSize = u_PointSize / gl_Position.w;  // perspective-correct size
    
    // Remap depth to [0, 1] loosely for turbo colormap or just pass it
    vDepth = gl_Position.z / gl_Position.w;
    
    vColor = max(aColorBase * 0.28209 + 0.5, 0.0);
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
in float vDepth;
in vec3 vColor;
out vec4 FragColor;

// A simple colormap function for depth (from implementation plan)
vec3 turbo(float t) {
    const vec3 c0 = vec3(0.114087, 0.0628834, 0.25482);
    const vec3 c1 = vec3(1.30906, 0.11718, 1.15781);
    const vec3 c2 = vec3(-8.85093, 2.76615, -4.95726);
    const vec3 c3 = vec3(26.6669, -7.50285, 2.50284);
    const vec3 c4 = vec3(-34.8211, 8.43477, 4.41708);
    const vec3 c5 = vec3(20.3013, -3.87063, -4.38289);
    const vec3 c6 = vec3(-4.42398, 0.0, 1.0); // Simplified a bit for brevity
    return clamp(c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6))))), 0.0, 1.0);
}

void main()
{
    // Circular splat
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    if (dot(coord, coord) > 1.0) discard;
    
    // You can mix vColor and turbo map, but let's use turbo(vDepth) or vColor based on plan
    // If vColor is valid, we can blend it. The plan says turbo(depth).
    // It's nice to blend or just use depth for sparse cloud:
    // Let's use the actual point colors if we have them, otherwise depth.
    // For now we'll just mix them for a cool effect, or use turbo.
    FragColor = vec4(turbo(clamp(vDepth * 0.5 + 0.5, 0.0, 1.0)) * 0.5 + vColor * 0.5, 1.0);
}
)";

SceneRenderer::SceneRenderer() {}

SceneRenderer::~SceneRenderer() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_ShaderProgram) glDeleteProgram(m_ShaderProgram);
}

bool SceneRenderer::Init() {
    openSplat_glDrawArrays = (openSplat_PFNGLDRAWARRAYSPROC)glfwGetProcAddress("glDrawArrays");
    openSplat_glUniform1f = (openSplat_PFNGLUNIFORM1FPROC)glfwGetProcAddress("glUniform1f");

    if (!openSplat_glDrawArrays) {
        OPENSPLAT_CORE_ERROR("Failed to load glDrawArrays pointer via GLFW!");
        return false;
    }

    if (!CreateShaderProgram()) return false;

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    
    // Layout corresponding to struct Gaussian
    // vec3 position, float scalar1 (16 bytes)
    // quat rotation (16 bytes)
    // vec3 scale, float opacity (16 bytes)
    // vec3 sh_coeffs[16] (we want the first one at offset 48)
    
    size_t stride = sizeof(Core::Gaussian);

    // Position (vec3 at offset 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // SH Coeffs [0] (vec3 at offset 16 + 16 + 16 = 48)
    size_t shOffset = offsetof(Core::Gaussian, sh_coeffs[0]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)shOffset);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    
    return true;
}

void SceneRenderer::UpdateSceneData(Core::GaussianScene& scene) {
    size_t count = scene.GetCount();
    if (count == 0 || !scene.Data()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(Core::Gaussian), scene.Data(), GL_DYNAMIC_DRAW);
    m_UploadedPointCount = count;
    
    // OPENSPLAT_CORE_TRACE("Uploaded {0} points to GPU for rendering", count);
}

void SceneRenderer::UpdateSceneData(const std::vector<Core::Gaussian>& points) {
    size_t count = points.size();
    if (count == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(Core::Gaussian), points.data(), GL_DYNAMIC_DRAW);
    m_UploadedPointCount = count;
}

void SceneRenderer::Render(const glm::mat4& viewProj, int width, int height) {
    if (m_UploadedPointCount == 0) return;

    glUseProgram(m_ShaderProgram);

    int loc = glGetUniformLocation(m_ShaderProgram, "u_ViewProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &viewProj[0][0]);
    
    int sizeLoc = glGetUniformLocation(m_ShaderProgram, "u_PointSize");
    if (openSplat_glUniform1f) {
        openSplat_glUniform1f(sizeLoc, m_PointSize);
    }

    glEnable(GL_PROGRAM_POINT_SIZE);
    
    glBindVertexArray(m_VAO);
    if (openSplat_glDrawArrays) {
        openSplat_glDrawArrays(GL_POINTS, 0, (GLsizei)m_UploadedPointCount);
    }
    glBindVertexArray(0);
}

unsigned int SceneRenderer::CompileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char message[1024];
        glGetShaderInfoLog(id, length, &length, message);
        OPENSPLAT_CORE_ERROR("Failed to compile shader: {0}", message);
        glDeleteShader(id);
        return 0;
    }
    return id;
}

bool SceneRenderer::CreateShaderProgram() {
    m_ShaderProgram = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (!vs || !fs) return false;

    glAttachShader(m_ShaderProgram, vs);
    glAttachShader(m_ShaderProgram, fs);
    glLinkProgram(m_ShaderProgram);

    int result;
    glGetProgramiv(m_ShaderProgram, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        char message[1024];
        glGetProgramInfoLog(m_ShaderProgram, 512, NULL, message);
        OPENSPLAT_CORE_ERROR("Failed to link shader program: {0}", message);
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    glBindVertexArray(0);

    return true;
}

// ==========================================
// GRID RENDERER IMPLEMENTATION
// ==========================================

static const char* gridVsSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 u_ViewProj;
out vec3 vColor;

void main()
{
    gl_Position = u_ViewProj * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char* gridFsSource = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
)";

GridRenderer::GridRenderer() {}

GridRenderer::~GridRenderer() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_ShaderProgram) glDeleteProgram(m_ShaderProgram);
}

unsigned int GridRenderer::CompileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        OPENSPLAT_CORE_ERROR("Grid Shader Compilation Error: {0}", infoLog);
    }
    return shader;
}

bool GridRenderer::CreateShaderProgram() {
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, gridVsSource);
    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, gridFsSource);

    m_ShaderProgram = glCreateProgram();
    glAttachShader(m_ShaderProgram, vertexShader);
    glAttachShader(m_ShaderProgram, fragmentShader);
    glLinkProgram(m_ShaderProgram);

    int success;
    glGetProgramiv(m_ShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_ShaderProgram, 512, nullptr, infoLog);
        OPENSPLAT_CORE_ERROR("Grid Program Linking Error: {0}", infoLog);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

bool GridRenderer::Init() {
    if (!CreateShaderProgram()) return false;

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    std::vector<float> vertices;
    auto addLine = [&](float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b) {
        vertices.push_back(x1); vertices.push_back(y1); vertices.push_back(z1);
        vertices.push_back(r); vertices.push_back(g); vertices.push_back(b);
        vertices.push_back(x2); vertices.push_back(y2); vertices.push_back(z2);
        vertices.push_back(r); vertices.push_back(g); vertices.push_back(b);
    };

    // Draw grid lines
    int gridSize = 10;
    for (int i = -gridSize; i <= gridSize; ++i) {
        if (i == 0) continue; // Skip main axes to draw them distinctly
        float col = 0.3f;
        // Lines parallel to Z
        addLine(i, 0.0f, -gridSize, i, 0.0f, gridSize, col, col, col);
        // Lines parallel to X
        addLine(-gridSize, 0.0f, i, gridSize, 0.0f, i, col, col, col);
    }

    // Main Axes
    // X Axis - Red
    addLine(-gridSize, 0.0f, 0.0f, gridSize, 0.0f, 0.0f, 0.8f, 0.2f, 0.2f);
    // Y Axis - Green
    addLine(0.0f, -gridSize, 0.0f, 0.0f, gridSize, 0.0f, 0.2f, 0.8f, 0.2f);
    // Z Axis - Blue
    addLine(0.0f, 0.0f, -gridSize, 0.0f, 0.0f, gridSize, 0.2f, 0.2f, 0.8f);
    
    // Draw thick arrow tips for Gizmo orientation visual logic
    // X Arrow
    addLine(gridSize, 0.0f, 0.0f, gridSize - 0.2f, 0.2f, 0.0f, 0.8f, 0.2f, 0.2f);
    addLine(gridSize, 0.0f, 0.0f, gridSize - 0.2f, -0.2f, 0.0f, 0.8f, 0.2f, 0.2f);
    // Y Arrow
    addLine(0.0f, gridSize, 0.0f, 0.2f, gridSize - 0.2f, 0.0f, 0.2f, 0.8f, 0.2f);
    addLine(0.0f, gridSize, 0.0f, -0.2f, gridSize - 0.2f, 0.0f, 0.2f, 0.8f, 0.2f);
    // Z Arrow
    addLine(0.0f, 0.0f, gridSize, 0.2f, 0.0f, gridSize - 0.2f, 0.2f, 0.2f, 0.8f);
    addLine(0.0f, 0.0f, gridSize, -0.2f, 0.0f, gridSize - 0.2f, 0.2f, 0.2f, 0.8f);

    m_VertexCount = vertices.size() / 6;

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Position attrib
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attrib
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

void GridRenderer::Render(const glm::mat4& viewProj) {
    if (m_VertexCount == 0) return;

    glUseProgram(m_ShaderProgram);

    int loc = glGetUniformLocation(m_ShaderProgram, "u_ViewProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &viewProj[0][0]);

    glBindVertexArray(m_VAO);
    if (openSplat_glDrawArrays) {
        openSplat_glDrawArrays(GL_LINES, 0, (int)m_VertexCount);
    }
    glBindVertexArray(0);
}

} // namespace Editor
} // namespace OpenSplat
