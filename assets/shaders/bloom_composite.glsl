/*
 * Bloom Composite Shader
 * Final pass: combines the original HDR scene with the blurred bloom,
 * then applies exposure-based tone mapping and gamma correction.
 */
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_Scene;
uniform sampler2D u_Bloom;
uniform float u_BloomStrength;
uniform float u_Exposure;

void main() {
    vec3 scene = texture(u_Scene, TexCoord).rgb;
    vec3 bloom = texture(u_Bloom, TexCoord).rgb;
    
    vec3 color = scene + bloom * u_BloomStrength;
    
    // Tone mapping: HDR to LDR using exposure
    color = vec3(1.0) - exp(-color * u_Exposure);
    
    // Gamma correction for display
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, 1.0);
}
