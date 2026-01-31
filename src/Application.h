#ifndef APPLICATION_H
#define APPLICATION_H

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "BloomRenderer.h"
#include "NoiseTexture.h"
#include "ScreenshotExporter.h"
#include "Shader.h"
#include "StarfieldCubemap.h"

struct BlackHoleParams {
  float radius = 0.5f;
  float diskInnerRadius = 1.0f;
  float diskOuterRadius = 4.0f;
  float diskThickness = 0.3f;
  glm::vec3 diskColor1 = glm::vec3(1.0f, 0.6f, 0.1f);
  glm::vec3 diskColor2 = glm::vec3(0.8f, 0.2f, 0.05f);
  float glowIntensity = 1.0f;
  float diskSpeed = 0.5f;
  float cameraDistance = 10.0f;
  float cameraAngle = 0.5f;
};

class Application {
public:
  Application();
  ~Application();

  // Initialize the application (window, OpenGL, ImGui, resources)
  bool init(int width, int height, const char *title);

  // Run the main loop
  void run();

  // Shutdown and cleanup
  void shutdown();

private:
  // Callbacks (static to work with GLFW C API)
  static void framebufferSizeCallback(GLFWwindow *window, int width,
                                      int height);
  static void scrollCallback(GLFWwindow *window, double xoffset,
                             double yoffset);
  static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
  static void mouseButtonCallback(GLFWwindow *window, int button, int action,
                                  int mods);

  void processInput();
  void renderUI();
  void renderScene();

  GLFWwindow *m_window = nullptr;
  int m_width = 1280;
  int m_height = 720;

  // Shaders
  Shader *m_blackHoleShader = nullptr;

  // Subsystems
  BloomRenderer m_bloomRenderer;
  ScreenshotExporter m_screenshotExporter;
  NoiseTexture m_noiseTexture;
  StarfieldCubemap m_starfieldCubemap;

  // Parameters
  BlackHoleParams m_params;
  BloomParams m_bloomParams;

  // Timing
  float m_fps = 0.0f;
  float m_frameTime = 0.0f;
  float m_lastFrameTime = 0.0f;
  float m_diskPhase = 0.0f;
  bool m_showFPS = false;

  // Rendering resources
  unsigned int m_quadVAO = 0;
  unsigned int m_quadVBO = 0;

  // Mouse camera control
  bool m_isDragging = false;
  double m_lastMouseX = 0.0;
  double m_lastMouseY = 0.0;
};

#endif // APPLICATION_H
