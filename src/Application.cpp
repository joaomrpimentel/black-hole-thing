#include "Application.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Static instance pointer for GLFW callbacks
static Application *s_instance = nullptr;

Application::Application() { s_instance = this; }

Application::~Application() {
  shutdown();
  s_instance = nullptr;
}

bool Application::init(int width, int height, const char *title) {
  m_width = width;
  m_height = height;

  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return false;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);

  m_window = glfwCreateWindow(width, height, title, NULL, NULL);
  if (!m_window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(m_window);
  glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
  glfwSetScrollCallback(m_window, scrollCallback);
  glfwSetCursorPosCallback(m_window, cursorPosCallback);
  glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
  glfwSwapInterval(1);

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return false;
  }

  glEnable(GL_MULTISAMPLE);

  // Initialize ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 8.0f;
  style.FrameRounding = 4.0f;
  style.GrabRounding = 4.0f;
  ImGui_ImplGlfw_InitForOpenGL(m_window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // Initialize subsystems
  m_bloomRenderer.init(width, height);
  m_screenshotExporter.init();
  m_blackHoleRenderer.init(width, height);

  return true;
}

void Application::run() {
  while (!glfwWindowShouldClose(m_window)) {
    float currentTime = (float)glfwGetTime();
    m_frameTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;
    m_fps = 1.0f / m_frameTime;

    // Update simulation
    m_blackHoleRenderer.update(m_frameTime);

    processInput();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    renderUI();
    renderScene();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
    glfwPollEvents();
  }
}

void Application::shutdown() {
  m_blackHoleRenderer.shutdown();
  
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  if (m_window) {
    glfwDestroyWindow(m_window);
    m_window = nullptr;
  }
  glfwTerminate();
}

void Application::framebufferSizeCallback(GLFWwindow *window, int width,
                                          int height) {
  if (s_instance) {
    s_instance->m_width = width;
    s_instance->m_height = height;
    glViewport(0, 0, width, height);
    s_instance->m_bloomRenderer.resize(width, height);
  }
}

void Application::scrollCallback(GLFWwindow *window, double xoffset,
                                 double yoffset) {
  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureMouse)
    return;

  if (s_instance) {
    auto& params = s_instance->m_blackHoleRenderer.getCameraParams();
    params.distance -= (float)yoffset * 0.5f;
    params.distance = glm::clamp(params.distance, 5.0f, 30.0f);
  }
}

void Application::cursorPosCallback(GLFWwindow *window, double xpos,
                                    double ypos) {
  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureMouse)
    return;

  if (s_instance && s_instance->m_isDragging) {
    double dx = xpos - s_instance->m_lastMouseX;
    double dy = ypos - s_instance->m_lastMouseY;

    // Adjust camera angle based on vertical drag
    auto& params = s_instance->m_blackHoleRenderer.getCameraParams();
    params.angle += (float)dy * 0.005f;
    params.angle = glm::clamp(params.angle, -1.57f, 1.57f);

    s_instance->m_lastMouseX = xpos;
    s_instance->m_lastMouseY = ypos;
  }
}

void Application::mouseButtonCallback(GLFWwindow *window, int button,
                                      int action, int mods) {
  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureMouse)
    return;

  if (s_instance && button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      s_instance->m_isDragging = true;
      glfwGetCursorPos(window, &s_instance->m_lastMouseX,
                       &s_instance->m_lastMouseY);
    } else if (action == GLFW_RELEASE) {
      s_instance->m_isDragging = false;
    }
  }
}

void Application::processInput() {
  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(m_window, true);
  }
}

void Application::renderUI() {
  ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
  ImGui::Begin("Black Hole Controls", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize);

  BlackHoleParams& params = m_blackHoleRenderer.getParams();
  CameraParams& camParams = m_blackHoleRenderer.getCameraParams();

  ImGui::SeparatorText("Black Hole");
  ImGui::SliderFloat("Radius", &params.radius, 0.1f, 2.0f);
  ImGui::SliderFloat("Glow Intensity", &params.glowIntensity, 0.0f, 3.0f);

  ImGui::SeparatorText("Accretion Disk");
  ImGui::SliderFloat("Inner Radius", &params.diskInnerRadius, 0.5f, 3.0f);
  ImGui::SliderFloat("Outer Radius", &params.diskOuterRadius, 2.0f, 8.0f);
  ImGui::SliderFloat("Thickness", &params.diskThickness, 0.05f, 1.0f);
  ImGui::SliderFloat("Speed", &params.diskSpeed, 0.0f, 10.0f);
  ImGui::ColorEdit3("Hot Color", &params.diskColor1[0]);
  ImGui::ColorEdit3("Cool Color", &params.diskColor2[0]);

  ImGui::SeparatorText("Camera");
  ImGui::SliderFloat("Distance", &camParams.distance, 5.0f, 30.0f);
  ImGui::SliderFloat("Angle", &camParams.angle, -1.57f, 1.57f);
  ImGui::Text("(Drag to orbit, Scroll to zoom)");

  ImGui::SeparatorText("Bloom");
  ImGui::Checkbox("Enable Bloom", &m_bloomParams.enabled);
  ImGui::SliderFloat("Threshold", &m_bloomParams.threshold, 0.0f, 2.0f);
  ImGui::SliderFloat("Intensity", &m_bloomParams.intensity, 0.0f, 3.0f);
  ImGui::SliderFloat("Strength", &m_bloomParams.strength, 0.0f, 2.0f);
  ImGui::SliderFloat("Exposure", &m_bloomParams.exposure, 0.5f, 3.0f);

  ImGui::Separator();
  ImGui::Checkbox("Show FPS", &m_showFPS);
  if (m_showFPS) {
    ImGui::Text("FPS: %.1f (%.2f ms)", m_fps, m_frameTime * 1000.0f);
  }

  ImGui::Separator();
  if (ImGui::Button("Export Image (1920x1080)", ImVec2(-1, 40))) {
    Shader compositeShader("assets/shaders/vertex.glsl",
                           "assets/shaders/bloom_composite.glsl");
    m_screenshotExporter.capture(1920, 1080, *m_blackHoleRenderer.getShader(),
                                 compositeShader, params, camParams, m_blackHoleRenderer.getDiskPhase(),
                                 m_bloomParams.exposure);
    glViewport(0, 0, m_width, m_height);
  }
  if (ImGui::Button("Export Image (4K)", ImVec2(-1, 40))) {
    Shader compositeShader("assets/shaders/vertex.glsl",
                           "assets/shaders/bloom_composite.glsl");
    m_screenshotExporter.capture(3840, 2160, *m_blackHoleRenderer.getShader(),
                                 compositeShader, params, camParams, m_blackHoleRenderer.getDiskPhase(),
                                 m_bloomParams.exposure);
    glViewport(0, 0, m_width, m_height);
  }

  ImGui::End();
}

void Application::renderScene() {
  // Render scene to bloom FBO
  glBindFramebuffer(GL_FRAMEBUFFER, m_bloomRenderer.getSceneFBO());
  glViewport(0, 0, m_width, m_height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Render the black hole
  m_blackHoleRenderer.render((float)glfwGetTime(), m_width, m_height);

  // Apply post-processing
  unsigned int quadVAO = m_blackHoleRenderer.getQuadVAO();
  if (m_bloomParams.enabled) {
    m_bloomRenderer.applyBloom(m_bloomParams, quadVAO);
  } else {
    m_bloomRenderer.renderWithoutBloom(m_bloomParams, quadVAO);
  }
}
