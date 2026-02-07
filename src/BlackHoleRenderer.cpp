#include "BlackHoleRenderer.h"
#include <iostream>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

BlackHoleRenderer::BlackHoleRenderer() {}

BlackHoleRenderer::~BlackHoleRenderer() {
    shutdown();
}

void BlackHoleRenderer::init(int width, int height) {
    if (m_initialized) return;

    // Load shaders
    m_shader = new Shader("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl");

    // Initialize quad for rendering
    initQuad();

    // Generate 3D noise texture (128^3 RGBA)
    m_noiseTexture.generate(128);

    // Generate starfield cubemap (2048x2048 per face)
    m_starfieldCubemap.init(2048);

    m_initialized = true;
}

void BlackHoleRenderer::initQuad() {
    // Standard full-screen quad
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void BlackHoleRenderer::update(float deltaTime) {
    m_diskPhase += deltaTime * m_params.diskSpeed;
}

void BlackHoleRenderer::render(float time, int width, int height) {
    if (!m_initialized) return;

    m_shader->use();
    m_shader->setVec2("u_Resolution", glm::vec2(width, height));
    m_shader->setFloat("u_Time", time);
    m_shader->setFloat("u_BlackHoleRadius", m_params.radius);
    m_shader->setFloat("u_DiskInnerRadius", m_params.diskInnerRadius);
    m_shader->setFloat("u_DiskOuterRadius", m_params.diskOuterRadius);
    m_shader->setFloat("u_DiskThickness", m_params.diskThickness);
    m_shader->setVec3("u_DiskColor1", m_params.diskColor1);
    m_shader->setVec3("u_DiskColor2", m_params.diskColor2);
    m_shader->setFloat("u_GlowIntensity", m_params.glowIntensity);
    m_shader->setFloat("u_DiskPhase", m_diskPhase);
    m_shader->setFloat("u_CameraDistance", m_cameraParams.distance);
    m_shader->setFloat("u_CameraAngle", m_cameraParams.angle);

    m_noiseTexture.bind(2);
    m_shader->setInt("u_NoiseTexture", 2);

    m_starfieldCubemap.bind(3);
    m_shader->setInt("u_StarfieldCubemap", 3);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void BlackHoleRenderer::shutdown() {
    if (!m_initialized) return;

    if (m_shader) {
        delete m_shader;
        m_shader = nullptr;
    }

    if (m_quadVAO) {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    
    if (m_quadVBO) {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }

    m_initialized = false;
}
