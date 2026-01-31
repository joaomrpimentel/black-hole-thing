/*
 * Kawase Bloom Blur Shader
 * More efficient blur using diagonal sampling with progressive offsets.
 * Achieves similar quality to Gaussian with fewer passes and texture samples.
 */
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_Image;
uniform float u_Offset;  
uniform float u_BloomIntensity;

void main() {
    vec2 texelSize = 1.0 / textureSize(u_Image, 0);
    
    vec3 result = texture(u_Image, TexCoord).rgb * 4.0;
    
    float offset = u_Offset + 0.5;
    result += texture(u_Image, TexCoord + vec2(-offset, offset) * texelSize).rgb;
    result += texture(u_Image, TexCoord + vec2(offset, offset) * texelSize).rgb;
    result += texture(u_Image, TexCoord + vec2(offset, -offset) * texelSize).rgb;
    result += texture(u_Image, TexCoord + vec2(-offset, -offset) * texelSize).rgb;
    
    FragColor = vec4(result / 8.0 * u_BloomIntensity, 1.0);
}
