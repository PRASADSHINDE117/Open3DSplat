#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace OpenSplat {
namespace Editor {

class ArcballCamera {
public:
  ArcballCamera(const glm::vec3 &target = glm::vec3(0.0f),
                float distance = 5.0f);

  void Update(int width, int height);

  // Inputs (Call these from GLFW callbacks)
  void Pitch(float angle);
  void Yaw(float angle);
  void Zoom(float delta);
  void Pan(float dx, float dy); // Screen space delta

  // Setters / Getters
  void SetTarget(const glm::vec3 &target) { m_Target = target; }
  void SetDistance(float distance) { m_Distance = distance; }

  glm::mat4 GetViewMatrix() const;
  glm::mat4 GetProjectionMatrix() const { return m_ProjectionMatrix; }
  glm::mat4 GetViewProjMatrix() const {
    return m_ProjectionMatrix * GetViewMatrix();
  }

  glm::vec3 GetPosition() const;

private:
  glm::vec3 m_Target;
  float m_Distance;
  glm::quat m_Rotation;

  glm::mat4 m_ProjectionMatrix;

  int m_ViewportWidth = 1;
  int m_ViewportHeight = 1;

  float m_Fov = 45.0f;
  float m_NearClip = 0.1f;
  float m_FarClip = 1000.0f;
};

} // namespace Editor
} // namespace OpenSplat
