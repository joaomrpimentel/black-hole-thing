#include "ScreenshotExporter.h"
#include "Shader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <ctime>
#include <iostream>
#include <vector>



ScreenshotExporter::ScreenshotExporter() {}

ScreenshotExporter::~ScreenshotExporter() { deleteResources(); }

void ScreenshotExporter::init() {
  // Create a reusable quad VAO
  float quadVertices[] = {-1.0f, 1.0f,  0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                          1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 1.0f,
                          1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f};

  glGenVertexArrays(1, &m_quadVAO);
  glGenBuffers(1, &m_quadVBO);
  glBindVertexArray(m_quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);

  m_initialized = true;
}

void ScreenshotExporter::ensureSize(int width, int height) {
  if (m_allocatedWidth == width && m_allocatedHeight == height) {
    return; // Already the right size
  }

  // Delete old resources if they exist
  if (m_fbo != 0) {
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_texture);
    glDeleteFramebuffers(1, &m_hdrFBO);
    glDeleteTextures(1, &m_hdrTexture);
  }

  // Create output FBO (LDR, for final readback)
  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_texture, 0);

  // Create HDR FBO (for scene rendering before tone mapping)
  glGenFramebuffers(1, &m_hdrFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);
  glGenTextures(1, &m_hdrTexture);
  glBindTexture(GL_TEXTURE_2D, m_hdrTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_hdrTexture, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  m_allocatedWidth = width;
  m_allocatedHeight = height;
}

std::string ScreenshotExporter::capture(int width, int height,
                                        Shader &sceneShader,
                                        Shader &compositeShader,
                                        const BlackHoleParams &params,
                                        const CameraParams &camParams,
                                        float diskPhase, float exposure) {
  if (!m_initialized) {
    std::cerr << "ScreenshotExporter not initialized!" << std::endl;
    return "";
  }

  ensureSize(width, height);

  // Check framebuffer completeness
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return "";
  }

  // Render scene to HDR FBO
  glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);
  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  sceneShader.use();
  sceneShader.setVec2("u_Resolution", glm::vec2(width, height));
  sceneShader.setFloat("u_Time",
                       diskPhase); // Use diskPhase for consistent timing
  sceneShader.setFloat("u_BlackHoleRadius", params.radius);
  sceneShader.setFloat("u_DiskInnerRadius", params.diskInnerRadius);
  sceneShader.setFloat("u_DiskOuterRadius", params.diskOuterRadius);
  sceneShader.setFloat("u_DiskThickness", params.diskThickness);
  sceneShader.setVec3("u_DiskColor1", params.diskColor1);
  sceneShader.setVec3("u_DiskColor2", params.diskColor2);
  sceneShader.setFloat("u_GlowIntensity", params.glowIntensity);
  sceneShader.setFloat("u_DiskPhase", diskPhase);
  sceneShader.setFloat("u_CameraDistance", camParams.distance);
  sceneShader.setFloat("u_CameraAngle", camParams.angle);

  glBindVertexArray(m_quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // Tone map to LDR FBO
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glClear(GL_COLOR_BUFFER_BIT);

  compositeShader.use();
  compositeShader.setInt("u_Scene", 0);
  compositeShader.setInt("u_Bloom", 1);
  compositeShader.setFloat("u_BloomStrength", 0.0f);
  compositeShader.setFloat("u_Exposure", exposure);

  auto& texture = m_hdrTexture; // Use reference to avoid warning or errors, actually this line is not needed if we just use m_hdrTexture.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_hdrTexture);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // Read pixels
  std::vector<unsigned char> pixels(width * height * 3);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

  // Flip vertically (OpenGL reads bottom-to-top)
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

  // Write to disk
  if (stbi_write_png(filename, width, height, 3, flipped.data(), width * 3)) {
    std::cout << "Saved: " << filename << " (" << width << "x" << height << ")"
              << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return std::string(filename);
  } else {
    std::cerr << "Failed to save image!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return "";
  }
}

void ScreenshotExporter::deleteResources() {
  if (!m_initialized)
    return;

  if (m_fbo != 0) {
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_texture);
    glDeleteFramebuffers(1, &m_hdrFBO);
    glDeleteTextures(1, &m_hdrTexture);
  }

  glDeleteVertexArrays(1, &m_quadVAO);
  glDeleteBuffers(1, &m_quadVBO);

  m_initialized = false;
}
