#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <ctime>
#include <iostream>
#include <string>
#include <vector>

// Window dimensions
int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;

// Shader parameters
struct BlackHoleParams {
  float radius = 0.5f;
  float diskInnerRadius = 1.0f;
  float diskOuterRadius = 4.0f;
  float diskThickness = 0.3f;
  glm::vec3 diskColor1 = glm::vec3(1.0f, 0.6f, 0.1f);  // Hot inner
  glm::vec3 diskColor2 = glm::vec3(0.8f, 0.2f, 0.05f); // Cooler outer
  float glowIntensity = 1.0f;
  float cameraDistance = 10.0f;
  float cameraAngle = 0.5f; // Radians, viewing angle from above
} params;

// Function prototypes
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void saveScreenshot(int width, int height);

int main() {
  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  // OpenGL 3.3 Core
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

  // Create window
  GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                        "Black Hole Visualizer", NULL, NULL);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSwapInterval(1); // Enable vsync

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Enable multisampling
  glEnable(GL_MULTISAMPLE);

  // Initialize ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Setup ImGui style
  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 8.0f;
  style.FrameRounding = 4.0f;
  style.GrabRounding = 4.0f;

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // Create fullscreen quad
  float quadVertices[] = {// positions   // texcoords
                          -1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                          0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                          -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                          1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};

  unsigned int quadVAO, quadVBO;
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Load shaders
  Shader shader("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl");

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui Control Panel
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 450), ImGuiCond_FirstUseEver);
    ImGui::Begin("Black Hole Controls", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::SeparatorText("Black Hole");
    ImGui::SliderFloat("Radius", &params.radius, 0.1f, 2.0f);
    ImGui::SliderFloat("Glow Intensity", &params.glowIntensity, 0.0f, 3.0f);

    ImGui::SeparatorText("Accretion Disk");
    ImGui::SliderFloat("Inner Radius", &params.diskInnerRadius, 0.5f, 3.0f);
    ImGui::SliderFloat("Outer Radius", &params.diskOuterRadius, 2.0f, 8.0f);
    ImGui::SliderFloat("Thickness", &params.diskThickness, 0.05f, 1.0f);
    ImGui::ColorEdit3("Hot Color", &params.diskColor1[0]);
    ImGui::ColorEdit3("Cool Color", &params.diskColor2[0]);

    ImGui::SeparatorText("Camera");
    ImGui::SliderFloat("Distance", &params.cameraDistance, 5.0f, 30.0f);
    ImGui::SliderFloat("Angle", &params.cameraAngle, 0.0f, 1.57f);

    ImGui::Separator();
    if (ImGui::Button("Export Image (1920x1080)", ImVec2(-1, 40))) {
      saveScreenshot(1920, 1080);
    }
    if (ImGui::Button("Export Image (4K)", ImVec2(-1, 40))) {
      saveScreenshot(3840, 2160);
    }

    ImGui::End();

    // Render black hole
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    shader.setVec2("u_Resolution", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
    shader.setFloat("u_Time", (float)glfwGetTime());
    shader.setFloat("u_BlackHoleRadius", params.radius);
    shader.setFloat("u_DiskInnerRadius", params.diskInnerRadius);
    shader.setFloat("u_DiskOuterRadius", params.diskOuterRadius);
    shader.setFloat("u_DiskThickness", params.diskThickness);
    shader.setVec3("u_DiskColor1", params.diskColor1);
    shader.setVec3("u_DiskColor2", params.diskColor2);
    shader.setFloat("u_GlowIntensity", params.glowIntensity);
    shader.setFloat("u_CameraDistance", params.cameraDistance);
    shader.setFloat("u_CameraAngle", params.cameraAngle);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  SCR_WIDTH = width;
  SCR_HEIGHT = height;
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void saveScreenshot(int width, int height) {
  // Create FBO for high-res render
  unsigned int fbo, texture;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer not complete!" << std::endl;
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    return;
  }

  // Create shader for offscreen render
  Shader shader("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl");

  // Create quad for offscreen render
  float quadVertices[] = {-1.0f, 1.0f,  0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                          1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 1.0f,
                          1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f};

  unsigned int quadVAO, quadVBO;
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Render to FBO
  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  shader.use();
  shader.setVec2("u_Resolution", glm::vec2(width, height));
  shader.setFloat("u_Time", (float)glfwGetTime());
  shader.setFloat("u_BlackHoleRadius", params.radius);
  shader.setFloat("u_DiskInnerRadius", params.diskInnerRadius);
  shader.setFloat("u_DiskOuterRadius", params.diskOuterRadius);
  shader.setFloat("u_DiskThickness", params.diskThickness);
  shader.setVec3("u_DiskColor1", params.diskColor1);
  shader.setVec3("u_DiskColor2", params.diskColor2);
  shader.setFloat("u_GlowIntensity", params.glowIntensity);
  shader.setFloat("u_CameraDistance", params.cameraDistance);
  shader.setFloat("u_CameraAngle", params.cameraAngle);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // Read pixels
  std::vector<unsigned char> pixels(width * height * 3);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

  // Flip vertically (OpenGL reads bottom-up)
  std::vector<unsigned char> flipped(width * height * 3);
  for (int y = 0; y < height; y++) {
    memcpy(&flipped[y * width * 3], &pixels[(height - 1 - y) * width * 3],
           width * 3);
  }

  // Generate filename with timestamp
  time_t now = time(0);
  struct tm *timeinfo = localtime(&now);
  char filename[128];
  strftime(filename, sizeof(filename), "blackhole_%Y%m%d_%H%M%S.png", timeinfo);

  // Save image
  if (stbi_write_png(filename, width, height, 3, flipped.data(), width * 3)) {
    std::cout << "Saved: " << filename << " (" << width << "x" << height << ")"
              << std::endl;
  } else {
    std::cerr << "Failed to save image!" << std::endl;
  }

  // Cleanup
  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);
  glDeleteFramebuffers(1, &fbo);
  glDeleteTextures(1, &texture);

  // Restore viewport
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}
