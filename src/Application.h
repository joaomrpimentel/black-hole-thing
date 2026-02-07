#ifndef APPLICATION_H
#define APPLICATION_H

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "BloomRenderer.h"
#include "ScreenshotExporter.h"
#include "BlackHoleRenderer.h" // Includes Shader.h, NoiseTexture.h, StarfieldCubemap.h

class Application {
public:
  Application();
  ~Application();

  bool init(int width, int height, const char *title);
  void run();
  void shutdown();

private:
  static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
  static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
  static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
  static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

  void processInput();
  void renderUI();
  void renderScene();

  GLFWwindow *m_window = nullptr;
  int m_width = 1280;
  int m_height = 720;

  // Subsystems
  BlackHoleRenderer m_blackHoleRenderer;
  BloomRenderer m_bloomRenderer;
  ScreenshotExporter m_screenshotExporter;

  // Parameters
  BloomParams m_bloomParams;

  // Timing
  float m_fps = 0.0f;
  float m_frameTime = 0.0f;
  float m_lastFrameTime = 0.0f;
  bool m_showFPS = false;

  // Mouse camera control
  bool m_isDragging = false;
  double m_lastMouseX = 0.0;
  double m_lastMouseY = 0.0;
};

#endif // APPLICATION_H
