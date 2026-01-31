/*
 * Bloom Extract Shader
 * Extracts bright pixels for the bloom effect, using the alpha channel as a mask
 * to control which elements should bloom (disk, glow) vs not bloom (stars).
 */
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_Scene;
uniform float u_BloomThreshold;

void main() {
    vec4 sceneColor = texture(u_Scene, TexCoord);
    vec3 color = sceneColor.rgb;
    float bloomMask = sceneColor.a;
    
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    if (luminance > u_BloomThreshold && bloomMask > 0.01) {
        float soft = clamp((luminance - u_BloomThreshold) / (1.0 - u_BloomThreshold), 0.0, 1.0);
        FragColor = vec4(color * soft * bloomMask, 1.0);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
