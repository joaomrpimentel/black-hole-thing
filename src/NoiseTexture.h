#ifndef NOISE_TEXTURE_H
#define NOISE_TEXTURE_H

#include <glad/glad.h>

class NoiseTexture {
public:
  NoiseTexture();
  ~NoiseTexture();

  // Generate a 3D noise texture with multiple octaves in RGBA channels
  // R = base noise, G = 2x freq, B = 4x freq, A = alternate seed
  // Size should be power of 2 for seamless tiling (64, 128)
  void generate(int size = 128);

  // Bind the texture to a texture unit
  void bind(unsigned int unit = 0) const;

  unsigned int getTextureID() const { return m_textureID; }
  int getSize() const { return m_size; }

private:
  // 3D Simplex noise implementation (CPU-side for baking)
  float snoise3D(float x, float y, float z);

  unsigned int m_textureID = 0;
  int m_size = 0;
  bool m_initialized = false;
};

#endif // NOISE_TEXTURE_H
