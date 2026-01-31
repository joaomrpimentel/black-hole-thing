#ifndef SCREENSHOT_EXPORTER_H
#define SCREENSHOT_EXPORTER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

// Forward declaration
class Shader;

struct BlackHoleParams;

class ScreenshotExporter {
public:
  ScreenshotExporter();
  ~ScreenshotExporter();

  // Initialize resources. Call once after OpenGL context is created.
  void init();

  // Capture a screenshot at the specified resolution.
  // Uses a dedicated FBO and the provided shader/params.
  // Returns the filename on success, empty string on failure.
  std::string capture(int width, int height, Shader &sceneShader,
                      Shader &compositeShader, const BlackHoleParams &params,
                      float diskPhase, float exposure);

private:
  void ensureSize(int width, int height);
  void deleteResources();

  unsigned int m_fbo = 0;
  unsigned int m_texture = 0;
  unsigned int m_hdrFBO = 0;
  unsigned int m_hdrTexture = 0;
  unsigned int m_quadVAO = 0;
  unsigned int m_quadVBO = 0;

  int m_allocatedWidth = 0;
  int m_allocatedHeight = 0;
  bool m_initialized = false;
};

#endif // SCREENSHOT_EXPORTER_H
