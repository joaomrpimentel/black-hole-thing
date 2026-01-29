#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_Scene;
uniform float u_BloomThreshold;

void main() {
    vec3 color = texture(u_Scene, TexCoord).rgb;
    
    // Calculate luminance
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // Extract bright areas
    if (luminance > u_BloomThreshold) {
        // Soft knee: smooth transition
        float soft = luminance - u_BloomThreshold;
        soft = clamp(soft / (1.0 - u_BloomThreshold), 0.0, 1.0);
        FragColor = vec4(color * soft, 1.0);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
