#include "BloomRenderer.h"

BloomRenderer::BloomRenderer() {}

BloomRenderer::~BloomRenderer() { deleteResources(); }

void BloomRenderer::init(int width, int height) {
  m_width = width;
  m_height = height;

  // Load shaders
  m_extractShader = new Shader("assets/shaders/vertex.glsl",
                               "assets/shaders/bloom_extract.glsl");
  m_blurShader = new Shader("assets/shaders/vertex.glsl",
                            "assets/shaders/bloom_blur.glsl");
  m_compositeShader = new Shader("assets/shaders/vertex.glsl",
                                 "assets/shaders/bloom_composite.glsl");

  // Scene FBO (full resolution, HDR)
  glGenFramebuffers(1, &m_sceneFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
  glGenTextures(1, &m_sceneTexture);
  glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_sceneTexture, 0);

  // Bright pass FBO (half resolution)
  glGenFramebuffers(1, &m_brightFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, m_brightFBO);
  glGenTextures(1, &m_brightTexture);
  glBindTexture(GL_TEXTURE_2D, m_brightTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_brightTexture, 0);

  // Ping-pong FBOs for Gaussian blur (half resolution)
  glGenFramebuffers(2, m_pingpongFBO);
  glGenTextures(2, m_pingpongTextures);
  for (int i = 0; i < 2; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[i]);
    glBindTexture(GL_TEXTURE_2D, m_pingpongTextures[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_pingpongTextures[i], 0);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  m_initialized = true;
}

void BloomRenderer::resize(int width, int height) {
  if (!m_initialized)
    return;

  m_width = width;
  m_height = height;

  glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
               GL_FLOAT, NULL);

  glBindTexture(GL_TEXTURE_2D, m_brightTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0, GL_RGBA,
               GL_FLOAT, NULL);

  for (int i = 0; i < 2; i++) {
    glBindTexture(GL_TEXTURE_2D, m_pingpongTextures[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0,
                 GL_RGBA, GL_FLOAT, NULL);
  }
}

void BloomRenderer::applyBloom(const BloomParams &params,
                               unsigned int quadVAO) {
  glBindVertexArray(quadVAO);

  // Pass 1: Extract bright areas
  glBindFramebuffer(GL_FRAMEBUFFER, m_brightFBO);
  glViewport(0, 0, m_width / 2, m_height / 2);
  glClear(GL_COLOR_BUFFER_BIT);

  m_extractShader->use();
  m_extractShader->setInt("u_Scene", 0);
  m_extractShader->setFloat("u_BloomThreshold", params.threshold);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // Pass 2: Gaussian blur (ping-pong)
  bool horizontal = true;
  int blurPasses = 6;
  m_blurShader->use();
  m_blurShader->setFloat("u_BloomIntensity", params.intensity);

  for (int i = 0; i < blurPasses; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[horizontal ? 1 : 0]);
    glClear(GL_COLOR_BUFFER_BIT);
    m_blurShader->setBool("u_Horizontal", horizontal);
    m_blurShader->setInt("u_Image", 0);
    glBindTexture(GL_TEXTURE_2D, i == 0
                                     ? m_brightTexture
                                     : m_pingpongTextures[horizontal ? 0 : 1]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    horizontal = !horizontal;
  }

  // Pass 3: Composite
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, m_width, m_height);
  glClear(GL_COLOR_BUFFER_BIT);

  m_compositeShader->use();
  m_compositeShader->setInt("u_Scene", 0);
  m_compositeShader->setInt("u_Bloom", 1);
  m_compositeShader->setFloat("u_BloomStrength", params.strength);
  m_compositeShader->setFloat("u_Exposure", params.exposure);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, m_pingpongTextures[0]);

  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void BloomRenderer::renderWithoutBloom(const BloomParams &params,
                                       unsigned int quadVAO) {
  glBindVertexArray(quadVAO);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, m_width, m_height);
  glClear(GL_COLOR_BUFFER_BIT);

  m_compositeShader->use();
  m_compositeShader->setInt("u_Scene", 0);
  m_compositeShader->setInt("u_Bloom", 1);
  m_compositeShader->setFloat("u_BloomStrength", 0.0f);
  m_compositeShader->setFloat("u_Exposure", params.exposure);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, m_sceneTexture); // Dummy, not used

  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void BloomRenderer::deleteResources() {
  if (!m_initialized)
    return;

  glDeleteFramebuffers(1, &m_sceneFBO);
  glDeleteFramebuffers(1, &m_brightFBO);
  glDeleteFramebuffers(2, m_pingpongFBO);
  glDeleteTextures(1, &m_sceneTexture);
  glDeleteTextures(1, &m_brightTexture);
  glDeleteTextures(2, m_pingpongTextures);

  delete m_extractShader;
  delete m_blurShader;
  delete m_compositeShader;

  m_initialized = false;
}
