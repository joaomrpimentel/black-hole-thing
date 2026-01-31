/*
 * Starfield Cubemap Generator Shader
 * Renders the starfield to a cubemap face for pre-computation.
 * This runs once at startup to generate static star positions.
 */
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 u_FaceDirection;
uniform vec3 u_FaceUp;

const float PI = 3.14159265359;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}

void main() {
    vec2 uv = TexCoord * 2.0 - 1.0;
    
    vec3 right = normalize(cross(u_FaceDirection, u_FaceUp));
    vec3 up = normalize(cross(right, u_FaceDirection));
    
    vec3 rd = normalize(u_FaceDirection + uv.x * right + uv.y * up);
    
    vec3 col = vec3(0.0);
    
    float theta = atan(rd.z, rd.x);
    float phi = asin(clamp(rd.y, -1.0, 1.0));
    
    for (int i = 0; i < 4; i++) {
        float layerDist = 50.0 + float(i) * 40.0;
        vec2 sphereCoord = vec2(theta, phi) * layerDist * 0.5;
        
        vec2 id = floor(sphereCoord + float(i) * 7.3);
        float star = hash(id);
        
        if (star > 0.96) {
            float brightness = pow(star - 0.96, 0.4) * 40.0;
            vec2 starUV = fract(sphereCoord + float(i) * 7.3) - 0.5;
            
            float starSize = 0.008; 
            float d = length(starUV);
            brightness *= smoothstep(starSize, 0.0, d);
            
            vec3 starColor = mix(vec3(1.0, 0.95, 0.9), vec3(0.9, 0.95, 1.0), hash(id + 0.5));
            col += starColor * brightness;
        }
    }
    
    float nebula = fbm(rd.xy * 3.0 + rd.z * 2.0) * 0.1;
    col += vec3(0.08, 0.04, 0.12) * nebula;
    
    FragColor = vec4(col, 1.0);
}
