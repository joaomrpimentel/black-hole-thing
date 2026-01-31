#include "NoiseTexture.h"
#include <iostream>
#include <vector>

// Permutation table for simplex noise
static const int perm[512] = {
    151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140,
    36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120,
    234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
    88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71,
    134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133,
    230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161,
    1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130,
    116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250,
    124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227,
    47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44,
    154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98,
    108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251, 34,
    242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14,
    239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121,
    50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243,
    141, 128, 195, 78, 66, 215, 61, 156, 180,
    // Repeat for wrapping
    151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140,
    36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120,
    234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
    88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71,
    134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133,
    230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161,
    1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130,
    116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250,
    124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227,
    47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44,
    154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98,
    108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251, 34,
    242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14,
    239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121,
    50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243,
    141, 128, 195, 78, 66, 215, 61, 156, 180};

// Gradient vectors for 3D simplex noise
static const float grad3[12][3] = {
    {1, 1, 0},  {-1, 1, 0},  {1, -1, 0}, {-1, -1, 0}, {1, 0, 1},  {-1, 0, 1},
    {1, 0, -1}, {-1, 0, -1}, {0, 1, 1},  {0, -1, 1},  {0, 1, -1}, {0, -1, -1}};

static inline float dot3(const float *g, float x, float y, float z) {
  return g[0] * x + g[1] * y + g[2] * z;
}

static inline int fastfloor(float x) { return x > 0 ? (int)x : (int)x - 1; }

NoiseTexture::NoiseTexture() {}

NoiseTexture::~NoiseTexture() {
  if (m_textureID != 0) {
    glDeleteTextures(1, &m_textureID);
  }
}

float NoiseTexture::snoise3D(float x, float y, float z) {
  // Skewing factors for 3D
  const float F3 = 1.0f / 3.0f;
  const float G3 = 1.0f / 6.0f;

  float s = (x + y + z) * F3;
  int i = fastfloor(x + s);
  int j = fastfloor(y + s);
  int k = fastfloor(z + s);

  float t = (i + j + k) * G3;
  float X0 = i - t;
  float Y0 = j - t;
  float Z0 = k - t;
  float x0 = x - X0;
  float y0 = y - Y0;
  float z0 = z - Z0;

  int i1, j1, k1;
  int i2, j2, k2;

  if (x0 >= y0) {
    if (y0 >= z0) {
      i1 = 1;
      j1 = 0;
      k1 = 0;
      i2 = 1;
      j2 = 1;
      k2 = 0;
    } else if (x0 >= z0) {
      i1 = 1;
      j1 = 0;
      k1 = 0;
      i2 = 1;
      j2 = 0;
      k2 = 1;
    } else {
      i1 = 0;
      j1 = 0;
      k1 = 1;
      i2 = 1;
      j2 = 0;
      k2 = 1;
    }
  } else {
    if (y0 < z0) {
      i1 = 0;
      j1 = 0;
      k1 = 1;
      i2 = 0;
      j2 = 1;
      k2 = 1;
    } else if (x0 < z0) {
      i1 = 0;
      j1 = 1;
      k1 = 0;
      i2 = 0;
      j2 = 1;
      k2 = 1;
    } else {
      i1 = 0;
      j1 = 1;
      k1 = 0;
      i2 = 1;
      j2 = 1;
      k2 = 0;
    }
  }

  float x1 = x0 - i1 + G3;
  float y1 = y0 - j1 + G3;
  float z1 = z0 - k1 + G3;
  float x2 = x0 - i2 + 2.0f * G3;
  float y2 = y0 - j2 + 2.0f * G3;
  float z2 = z0 - k2 + 2.0f * G3;
  float x3 = x0 - 1.0f + 3.0f * G3;
  float y3 = y0 - 1.0f + 3.0f * G3;
  float z3 = z0 - 1.0f + 3.0f * G3;

  int ii = i & 255;
  int jj = j & 255;
  int kk = k & 255;

  int gi0 = perm[ii + perm[jj + perm[kk]]] % 12;
  int gi1 = perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]] % 12;
  int gi2 = perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]] % 12;
  int gi3 = perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]] % 12;

  float n0, n1, n2, n3;

  float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
  if (t0 < 0)
    n0 = 0.0f;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot3(grad3[gi0], x0, y0, z0);
  }

  float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
  if (t1 < 0)
    n1 = 0.0f;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot3(grad3[gi1], x1, y1, z1);
  }

  float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
  if (t2 < 0)
    n2 = 0.0f;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot3(grad3[gi2], x2, y2, z2);
  }

  float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
  if (t3 < 0)
    n3 = 0.0f;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot3(grad3[gi3], x3, y3, z3);
  }

  // Scale to [-1, 1]
  return 32.0f * (n0 + n1 + n2 + n3);
}

void NoiseTexture::generate(int size) {
  m_size = size;

  // RGBA: 4 channels, each with different noise characteristics
  // R = base noise (1x frequency)
  // G = 2x frequency
  // B = 4x frequency
  // A = different seed (for variation)
  std::vector<float> data(size * size * size * 4);

  std::cout << "Generating " << size << "^3 RGBA noise texture..." << std::endl;

  for (int z = 0; z < size; z++) {
    for (int y = 0; y < size; y++) {
      for (int x = 0; x < size; x++) {
        // Normalized coordinates [0, 1] mapped to noise space
        // We use a period that tiles seamlessly
        float fx = (float)x / (float)size;
        float fy = (float)y / (float)size;
        float fz = (float)z / (float)size;

        // Scale to create good detail (4 tiles across the texture)
        float scale = 4.0f;
        float px = fx * scale;
        float py = fy * scale;
        float pz = fz * scale;

        // R: Base noise (1x frequency), range [-1, 1] -> [0, 1]
        float r = snoise3D(px, py, pz);
        r = r * 0.5f + 0.5f;

        // G: 2x frequency
        float g = snoise3D(px * 2.0f, py * 2.0f, pz * 2.0f);
        g = g * 0.5f + 0.5f;

        // B: 4x frequency
        float b = snoise3D(px * 4.0f, py * 4.0f, pz * 4.0f);
        b = b * 0.5f + 0.5f;

        // A: Different seed (offset by large amount)
        float a = snoise3D(px + 100.0f, py + 200.0f, pz + 300.0f);
        a = a * 0.5f + 0.5f;

        int idx = (z * size * size + y * size + x) * 4;
        data[idx + 0] = r;
        data[idx + 1] = g;
        data[idx + 2] = b;
        data[idx + 3] = a;
      }
    }
  }

  // Upload to GPU as RGBA16F for precision
  glGenTextures(1, &m_textureID);
  glBindTexture(GL_TEXTURE_3D, m_textureID);

  glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, size, size, size, 0, GL_RGBA,
               GL_FLOAT, data.data());

  // Use linear filtering for smooth interpolation
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Seamless wrapping in all dimensions
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

  glBindTexture(GL_TEXTURE_3D, 0);
  m_initialized = true;

  std::cout << "Noise texture ready (" << size * size * size * 4 * 2 / 1024
            << " KB)" << std::endl;
}

void NoiseTexture::bind(unsigned int unit) const {
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_3D, m_textureID);
}
