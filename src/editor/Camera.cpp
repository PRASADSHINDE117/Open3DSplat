#include "editor/Camera.h"

namespace OpenSplat {
namespace Editor {

ArcballCamera::ArcballCamera(const glm::vec3& target, float distance)
    : m_Target(target), m_Distance(distance), m_Rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
}

void ArcballCamera::Update(int width, int height) {
    if (width > 0 && height > 0) {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        float aspect = (float)width / (float)height;
        m_ProjectionMatrix = glm::perspective(glm::radians(m_Fov), aspect, m_NearClip, m_FarClip);
    }
}

glm::mat4 ArcballCamera::GetViewMatrix() const {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Target) * glm::mat4_cast(m_Rotation) * glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, m_Distance));
    return glm::inverse(transform);
}

glm::vec3 ArcballCamera::GetPosition() const {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Target) * glm::mat4_cast(m_Rotation) * glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, m_Distance));
    return glm::vec3(transform[3]);
}

void ArcballCamera::Pitch(float angle) {
    glm::quat q = glm::angleAxis(angle, glm::vec3(1.0f, 0.0f, 0.0f));
    m_Rotation = m_Rotation * q;
    m_Rotation = glm::normalize(m_Rotation);
}

void ArcballCamera::Yaw(float angle) {
    // Global Y-axis for standard turntable yaw
    glm::quat q = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    m_Rotation = q * m_Rotation; 
    m_Rotation = glm::normalize(m_Rotation);
}

void ArcballCamera::Zoom(float delta) {
    m_Distance -= delta * m_Distance * 0.1f;
    if (m_Distance < 0.1f) m_Distance = 0.1f;
}

void ArcballCamera::Pan(float dx, float dy) {
    // Screen space panning mapped to world space
    float panSpeed = m_Distance * 0.001f;
    
    // Right and Up vectors of the camera
    glm::mat4 view = GetViewMatrix();
    glm::vec3 right = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 up = glm::vec3(view[0][1], view[1][1], view[2][1]);
    
    m_Target -= right * (dx * panSpeed);
    m_Target += up * (dy * panSpeed);
}

} // namespace Editor
} // namespace OpenSplat
