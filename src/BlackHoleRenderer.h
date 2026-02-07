#ifndef BLACK_HOLE_RENDERER_H
#define BLACK_HOLE_RENDERER_H

#include <glm/glm.hpp>
#include "Shader.h"
#include "NoiseTexture.h"
#include "StarfieldCubemap.h"

struct BlackHoleParams {
    float radius = 0.5f;
    float diskInnerRadius = 1.0f;
    float diskOuterRadius = 4.0f;
    float diskThickness = 0.3f;
    glm::vec3 diskColor1 = glm::vec3(1.0f, 0.6f, 0.1f);
    glm::vec3 diskColor2 = glm::vec3(0.8f, 0.2f, 0.05f);
    float glowIntensity = 1.0f;
    float diskSpeed = 0.5f;
};

struct CameraParams {
    float distance = 10.0f;
    float angle = 0.5f;
};

class BlackHoleRenderer {
public:
    BlackHoleRenderer();
    ~BlackHoleRenderer();

    void init(int width, int height);
    void render(float time, int width, int height);
    void update(float deltaTime);
    void shutdown();

    BlackHoleParams& getParams() { return m_params; }
    CameraParams& getCameraParams() { return m_cameraParams; }
    Shader* getShader() { return m_shader; } // For screenshot export
    float getDiskPhase() const { return m_diskPhase; }
    unsigned int getQuadVAO() const { return m_quadVAO; }

private:
    void initQuad();

    BlackHoleParams m_params;
    CameraParams m_cameraParams;

    Shader* m_shader = nullptr;
    NoiseTexture m_noiseTexture;
    StarfieldCubemap m_starfieldCubemap;

    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;

    float m_diskPhase = 0.0f;
    bool m_initialized = false;
};

#endif
