#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_Image;
uniform bool u_Horizontal;
uniform float u_BloomIntensity;

// Gaussian weights for 9-tap blur
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texelSize = 1.0 / textureSize(u_Image, 0);
    vec3 result = texture(u_Image, TexCoord).rgb * weights[0];
    
    if (u_Horizontal) {
        for (int i = 1; i < 5; ++i) {
            result += texture(u_Image, TexCoord + vec2(texelSize.x * float(i), 0.0)).rgb * weights[i];
            result += texture(u_Image, TexCoord - vec2(texelSize.x * float(i), 0.0)).rgb * weights[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            result += texture(u_Image, TexCoord + vec2(0.0, texelSize.y * float(i))).rgb * weights[i];
            result += texture(u_Image, TexCoord - vec2(0.0, texelSize.y * float(i))).rgb * weights[i];
        }
    }
    
    FragColor = vec4(result * u_BloomIntensity, 1.0);
}
