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
  glm::vec3 diskColor1 = glm::vec3(1.0f, 0.6f, 0.1f);
  glm::vec3 diskColor2 = glm::vec3(0.8f, 0.2f, 0.05f);
  float glowIntensity = 1.0f;
  float cameraDistance = 10.0f;
  float cameraAngle = 0.5f;
} params;

// Bloom parameters
struct BloomParams {
  float threshold = 0.8f;
  float intensity = 1.0f;
  float strength = 0.5f;
  float exposure = 1.2f;
  bool enabled = true;
} bloomParams;

// Framebuffer objects for bloom
struct BloomFBO {
  unsigned int sceneFBO, sceneTexture;
  unsigned int brightFBO, brightTexture;
  unsigned int pingpongFBO[2], pingpongTextures[2];
};
BloomFBO bloomFBO;

// Function prototypes
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void saveScreenshot(int width, int height);
void initBloomFBOs(int width, int height);
void resizeBloomFBOs(int width, int height);
void deleteBloomFBOs();

void initBloomFBOs(int width, int height) {
  // Scene FBO (HDR)
  glGenFramebuffers(1, &bloomFBO.sceneFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO.sceneFBO);
  glGenTextures(1, &bloomFBO.sceneTexture);
  glBindTexture(GL_TEXTURE_2D, bloomFBO.sceneTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         bloomFBO.sceneTexture, 0);

  // Brightness extraction FBO
  glGenFramebuffers(1, &bloomFBO.brightFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO.brightFBO);
  glGenTextures(1, &bloomFBO.brightTexture);
  glBindTexture(GL_TEXTURE_2D, bloomFBO.brightTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         bloomFBO.brightTexture, 0);

  // Ping-pong FBOs for blur
  glGenFramebuffers(2, bloomFBO.pingpongFBO);
  glGenTextures(2, bloomFBO.pingpongTextures);
  for (int i = 0; i < 2; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO.pingpongFBO[i]);
    glBindTexture(GL_TEXTURE_2D, bloomFBO.pingpongTextures[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           bloomFBO.pingpongTextures[i], 0);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void resizeBloomFBOs(int width, int height) {
  glBindTexture(GL_TEXTURE_2D, bloomFBO.sceneTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
               GL_FLOAT, NULL);

  glBindTexture(GL_TEXTURE_2D, bloomFBO.brightTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0, GL_RGBA,
               GL_FLOAT, NULL);

  for (int i = 0; i < 2; i++) {
    glBindTexture(GL_TEXTURE_2D, bloomFBO.pingpongTextures[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0,
                 GL_RGBA, GL_FLOAT, NULL);
  }
}

void deleteBloomFBOs() {
  glDeleteFramebuffers(1, &bloomFBO.sceneFBO);
  glDeleteFramebuffers(1, &bloomFBO.brightFBO);
  glDeleteFramebuffers(2, bloomFBO.pingpongFBO);
  glDeleteTextures(1, &bloomFBO.sceneTexture);
  glDeleteTextures(1, &bloomFBO.brightTexture);
  glDeleteTextures(2, bloomFBO.pingpongTextures);
}

int main() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);

  GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                        "Black Hole Visualizer", NULL, NULL);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
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
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // Create fullscreen quad
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

  // Load shaders
  Shader blackHoleShader("assets/shaders/vertex.glsl",
                         "assets/shaders/fragment.glsl");
  Shader bloomExtractShader("assets/shaders/vertex.glsl",
                            "assets/shaders/bloom_extract.glsl");
  Shader bloomBlurShader("assets/shaders/vertex.glsl",
                         "assets/shaders/bloom_blur.glsl");
  Shader bloomCompositeShader("assets/shaders/vertex.glsl",
                              "assets/shaders/bloom_composite.glsl");

  // Initialize bloom FBOs
  initBloomFBOs(SCR_WIDTH, SCR_HEIGHT);

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Control Panel
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
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

    ImGui::SeparatorText("Bloom");
    ImGui::Checkbox("Enable Bloom", &bloomParams.enabled);
    ImGui::SliderFloat("Threshold", &bloomParams.threshold, 0.0f, 2.0f);
    ImGui::SliderFloat("Intensity", &bloomParams.intensity, 0.0f, 3.0f);
    ImGui::SliderFloat("Strength", &bloomParams.strength, 0.0f, 2.0f);
    ImGui::SliderFloat("Exposure", &bloomParams.exposure, 0.5f, 3.0f);

    ImGui::Separator();
    if (ImGui::Button("Export Image (1920x1080)", ImVec2(-1, 40))) {
      saveScreenshot(1920, 1080);
    }
    if (ImGui::Button("Export Image (4K)", ImVec2(-1, 40))) {
      saveScreenshot(3840, 2160);
    }

    ImGui::End();

    // === PASS 1: Render scene to HDR FBO ===
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO.sceneFBO);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    blackHoleShader.use();
    blackHoleShader.setVec2("u_Resolution", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
    blackHoleShader.setFloat("u_Time", (float)glfwGetTime());
    blackHoleShader.setFloat("u_BlackHoleRadius", params.radius);
    blackHoleShader.setFloat("u_DiskInnerRadius", params.diskInnerRadius);
    blackHoleShader.setFloat("u_DiskOuterRadius", params.diskOuterRadius);
    blackHoleShader.setFloat("u_DiskThickness", params.diskThickness);
    blackHoleShader.setVec3("u_DiskColor1", params.diskColor1);
    blackHoleShader.setVec3("u_DiskColor2", params.diskColor2);
    blackHoleShader.setFloat("u_GlowIntensity", params.glowIntensity);
    blackHoleShader.setFloat("u_CameraDistance", params.cameraDistance);
    blackHoleShader.setFloat("u_CameraAngle", params.cameraAngle);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    if (bloomParams.enabled) {
      // === PASS 2: Extract bright areas ===
      glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO.brightFBO);
      glViewport(0, 0, SCR_WIDTH / 2, SCR_HEIGHT / 2);
      glClear(GL_COLOR_BUFFER_BIT);

      bloomExtractShader.use();
      bloomExtractShader.setInt("u_Scene", 0);
      bloomExtractShader.setFloat("u_BloomThreshold", bloomParams.threshold);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, bloomFBO.sceneTexture);
      glDrawArrays(GL_TRIANGLES, 0, 6);

      // === PASS 3: Gaussian blur (ping-pong) ===
      bool horizontal = true;
      int blurPasses = 6;
      bloomBlurShader.use();
      bloomBlurShader.setFloat("u_BloomIntensity", bloomParams.intensity);

      for (int i = 0; i < blurPasses; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER,
                          bloomFBO.pingpongFBO[horizontal ? 1 : 0]);
        glClear(GL_COLOR_BUFFER_BIT);
        bloomBlurShader.setBool("u_Horizontal", horizontal);
        bloomBlurShader.setInt("u_Image", 0);
        glBindTexture(GL_TEXTURE_2D,
                      i == 0 ? bloomFBO.brightTexture
                             : bloomFBO.pingpongTextures[horizontal ? 0 : 1]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        horizontal = !horizontal;
      }

      // === PASS 4: Composite ===
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
      glClear(GL_COLOR_BUFFER_BIT);

      bloomCompositeShader.use();
      bloomCompositeShader.setInt("u_Scene", 0);
      bloomCompositeShader.setInt("u_Bloom", 1);
      bloomCompositeShader.setFloat("u_BloomStrength", bloomParams.strength);
      bloomCompositeShader.setFloat("u_Exposure", bloomParams.exposure);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, bloomFBO.sceneTexture);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, bloomFBO.pingpongTextures[0]);

      glDrawArrays(GL_TRIANGLES, 0, 6);
    } else {
      // No bloom: simple pass-through with tone mapping
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
      glClear(GL_COLOR_BUFFER_BIT);

      bloomCompositeShader.use();
      bloomCompositeShader.setInt("u_Scene", 0);
      bloomCompositeShader.setInt("u_Bloom", 1);
      bloomCompositeShader.setFloat("u_BloomStrength", 0.0f);
      bloomCompositeShader.setFloat("u_Exposure", bloomParams.exposure);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, bloomFBO.sceneTexture);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, bloomFBO.sceneTexture); // dummy

      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  deleteBloomFBOs();
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
  resizeBloomFBOs(width, height);
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

  // Render scene
  Shader shader("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl");
  Shader compositeShader("assets/shaders/vertex.glsl",
                         "assets/shaders/bloom_composite.glsl");

  // Temp FBO for HDR scene
  unsigned int hdrFBO, hdrTexture;
  glGenFramebuffers(1, &hdrFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
  glGenTextures(1, &hdrTexture);
  glBindTexture(GL_TEXTURE_2D, hdrTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         hdrTexture, 0);

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

  // Render to HDR FBO
  glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
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

  // Tone map to output FBO
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glClear(GL_COLOR_BUFFER_BIT);

  compositeShader.use();
  compositeShader.setInt("u_Scene", 0);
  compositeShader.setInt("u_Bloom", 1);
  compositeShader.setFloat("u_BloomStrength", 0.0f);
  compositeShader.setFloat("u_Exposure", bloomParams.exposure);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, hdrTexture);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // Read pixels
  std::vector<unsigned char> pixels(width * height * 3);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

  // Flip vertically
  std::vector<unsigned char> flipped(width * height * 3);
  for (int y = 0; y < height; y++) {
    memcpy(&flipped[y * width * 3], &pixels[(height - 1 - y) * width * 3],
           width * 3);
  }

  // Generate filename
  time_t now = time(0);
  struct tm *timeinfo = localtime(&now);
  char filename[128];
  strftime(filename, sizeof(filename), "blackhole_%Y%m%d_%H%M%S.png", timeinfo);

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
  glDeleteFramebuffers(1, &hdrFBO);
  glDeleteTextures(1, &texture);
  glDeleteTextures(1, &hdrTexture);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}
