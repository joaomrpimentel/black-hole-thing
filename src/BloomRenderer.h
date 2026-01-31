#ifndef BLOOM_RENDERER_H
#define BLOOM_RENDERER_H

#include "Shader.h"
#include <glad/glad.h>

struct BloomParams {
  float threshold = 0.8f;
  float intensity = 1.0f;
  float strength = 0.5f;
  float exposure = 1.2f;
  bool enabled = true;
};

class BloomRenderer {
public:
  BloomRenderer();
  ~BloomRenderer();

  // Initialize FBOs for a given resolution
  void init(int width, int height);

  // Resize FBOs when window size changes
  void resize(int width, int height);

  // Render the scene to the internal HDR FBO
  // Returns the scene texture ID for further processing
  unsigned int getSceneFBO() const { return m_sceneFBO; }
  unsigned int getSceneTexture() const { return m_sceneTexture; }

  // Apply bloom post-processing and render to default framebuffer
  void applyBloom(const BloomParams &params, unsigned int quadVAO);

  // Render without bloom (just tone mapping)
  void renderWithoutBloom(const BloomParams &params, unsigned int quadVAO);

private:
  void deleteResources();

  // FBO and texture handles
  unsigned int m_sceneFBO = 0;
  unsigned int m_sceneTexture = 0;
  unsigned int m_brightFBO = 0;
  unsigned int m_brightTexture = 0;
  unsigned int m_pingpongFBO[2] = {0, 0};
  unsigned int m_pingpongTextures[2] = {0, 0};

  // Shaders
  Shader *m_extractShader = nullptr;
  Shader *m_blurShader = nullptr;
  Shader *m_kawaseShader = nullptr;
  Shader *m_compositeShader = nullptr;

  int m_width = 0;
  int m_height = 0;
  bool m_initialized = false;
};

#endif // BLOOM_RENDERER_H
