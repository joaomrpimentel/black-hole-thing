#include "StarfieldCubemap.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

StarfieldCubemap::StarfieldCubemap() {}

StarfieldCubemap::~StarfieldCubemap() { deleteResources(); }

void StarfieldCubemap::init(int faceResolution) {
  m_faceResolution = faceResolution;

  m_generatorShader = new Shader("assets/shaders/vertex.glsl",
                                 "assets/shaders/starfield_cubemap.glsl");

  glGenTextures(1, &m_cubemapTexture);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);

  for (int i = 0; i < 6; i++) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                 faceResolution, faceResolution, 0, GL_RGB, GL_FLOAT, nullptr);
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glGenFramebuffers(1, &m_fbo);

  float quadVertices[] = {
      -1.0f, 1.0f,  0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
      1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 1.0f,
      1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f,
  };

  unsigned int quadVAO, quadVBO;
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));

  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glViewport(0, 0, faceResolution, faceResolution);

  for (int i = 0; i < 6; i++) {
    renderFace(i, quadVAO);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);

  m_initialized = true;
  std::cout << "Starfield cubemap generated (" << faceResolution << "x"
            << faceResolution << " per face)" << std::endl;
}

void StarfieldCubemap::renderFace(int face, unsigned int quadVAO) {
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                         m_cubemapTexture, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  // Define view directions for each cubemap face
  // +X, -X, +Y, -Y, +Z, -Z
  glm::vec3 directions[] = {
      glm::vec3(1.0f, 0.0f, 0.0f),  // +X
      glm::vec3(-1.0f, 0.0f, 0.0f), // -X
      glm::vec3(0.0f, 1.0f, 0.0f),  // +Y
      glm::vec3(0.0f, -1.0f, 0.0f), // -Y
      glm::vec3(0.0f, 0.0f, 1.0f),  // +Z
      glm::vec3(0.0f, 0.0f, -1.0f)  // -Z
  };

  glm::vec3 ups[] = {
      glm::vec3(0.0f, -1.0f, 0.0f), // +X
      glm::vec3(0.0f, -1.0f, 0.0f), // -X
      glm::vec3(0.0f, 0.0f, 1.0f),  // +Y
      glm::vec3(0.0f, 0.0f, -1.0f), // -Y
      glm::vec3(0.0f, -1.0f, 0.0f), // +Z
      glm::vec3(0.0f, -1.0f, 0.0f)  // -Z
  };

  m_generatorShader->use();
  m_generatorShader->setVec3("u_FaceDirection", directions[face]);
  m_generatorShader->setVec3("u_FaceUp", ups[face]);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void StarfieldCubemap::bind(int textureUnit) {
  glActiveTexture(GL_TEXTURE0 + textureUnit);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);
}

void StarfieldCubemap::deleteResources() {
  if (!m_initialized)
    return;

  glDeleteTextures(1, &m_cubemapTexture);
  glDeleteFramebuffers(1, &m_fbo);
  delete m_generatorShader;

  m_initialized = false;
}
