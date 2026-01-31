#ifndef STARFIELD_CUBEMAP_H
#define STARFIELD_CUBEMAP_H

#include "Shader.h"
#include <glad/glad.h>

class StarfieldCubemap {
public:
  StarfieldCubemap();
  ~StarfieldCubemap();

  // Initialize and generate the cubemap texture
  void init(int faceResolution = 512);

  // Bind the cubemap to a texture unit
  void bind(int textureUnit);

  // Get the cubemap texture ID
  unsigned int getTextureID() const { return m_cubemapTexture; }

private:
  void deleteResources();
  void renderFace(int face, unsigned int quadVAO);

  unsigned int m_cubemapTexture = 0;
  unsigned int m_fbo = 0;
  Shader *m_generatorShader = nullptr;
  int m_faceResolution = 512;
  bool m_initialized = false;
};

#endif // STARFIELD_CUBEMAP_H
