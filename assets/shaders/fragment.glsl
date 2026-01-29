#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// Uniforms
uniform vec2 u_Resolution;
uniform float u_Time;
uniform float u_BlackHoleRadius;
uniform float u_DiskInnerRadius;
uniform float u_DiskOuterRadius;
uniform float u_DiskThickness;
uniform vec3 u_DiskColor1;
uniform vec3 u_DiskColor2;
uniform float u_GlowIntensity;
uniform float u_CameraDistance;
uniform float u_CameraAngle;

// Constants
const float PI = 3.14159265359;
const float SCHWARZSCHILD_FACTOR = 1.5; // How much gravity bends light
const int MAX_STEPS = 200;
const float MAX_DIST = 100.0;
const float EPSILON = 0.001;

// Noise functions for disk texture
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

// Rotate vector around Y axis
vec3 rotateY(vec3 v, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

// Rotate vector around X axis
vec3 rotateX(vec3 v, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec3(v.x, c * v.y - s * v.z, s * v.y + c * v.z);
}

// Starfield background
vec3 getStars(vec3 rd) {
    vec3 col = vec3(0.0);
    
    // Multiple layers of stars
    for (int i = 0; i < 3; i++) {
        vec3 p = rd * (50.0 + float(i) * 30.0);
        vec2 id = floor(p.xy + p.z);
        float star = hash(id);
        
        if (star > 0.97) {
            float brightness = pow(star - 0.97, 0.5) * 30.0;
            vec3 starPos = vec3(fract(p.xy + p.z) - 0.5, 0.0);
            float d = length(starPos.xy);
            brightness *= smoothstep(0.05, 0.0, d);
            
            // Star color variation
            vec3 starColor = mix(vec3(1.0, 0.9, 0.8), vec3(0.8, 0.9, 1.0), hash(id + 0.5));
            col += starColor * brightness;
        }
    }
    
    // Subtle nebula
    float nebula = fbm(rd.xy * 3.0 + rd.z * 2.0) * 0.15;
    col += vec3(0.1, 0.05, 0.15) * nebula;
    
    return col;
}

// Sample the accretion disk
vec3 sampleDisk(vec3 pos, float distToCenter) {
    if (distToCenter < u_DiskInnerRadius || distToCenter > u_DiskOuterRadius) {
        return vec3(0.0);
    }
    
    // Normalized position in disk
    float t = (distToCenter - u_DiskInnerRadius) / (u_DiskOuterRadius - u_DiskInnerRadius);
    
    // Disk angle for animation
    float angle = atan(pos.z, pos.x);
    float diskNoise = fbm(vec2(angle * 3.0 + u_Time * 0.2, distToCenter * 2.0));
    
    // Temperature gradient (hotter near center)
    float temperature = 1.0 - t;
    temperature = pow(temperature, 0.5);
    
    // Color based on temperature
    vec3 color = mix(u_DiskColor2, u_DiskColor1, temperature);
    
    // Add noise variation
    color *= 0.5 + 0.5 * diskNoise;
    
    // Brightness falls off with radius
    float brightness = (1.0 - t) * (0.3 + 0.7 * diskNoise);
    
    // Doppler effect (simplified)
    float doppler = 0.5 + 0.5 * sin(angle + u_Time * 0.5);
    brightness *= 0.7 + 0.3 * doppler;
    
    return color * brightness * 2.0;
}

void main() {
    // Normalized coordinates
    vec2 uv = (gl_FragCoord.xy - 0.5 * u_Resolution) / min(u_Resolution.x, u_Resolution.y);
    
    // Camera setup
    float camDist = u_CameraDistance;
    float camAngle = u_CameraAngle;
    
    vec3 ro = vec3(0.0, 0.0, camDist); // Ray origin
    ro = rotateX(ro, camAngle);
    
    vec3 forward = normalize(-ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);
    
    vec3 rd = normalize(forward + uv.x * right + uv.y * up); // Ray direction
    
    // Ray marching with gravitational lensing
    vec3 pos = ro;
    vec3 vel = rd;
    
    vec3 color = vec3(0.0);
    float totalDist = 0.0;
    bool hitDisk = false;
    bool hitHorizon = false;
    
    float prevY = pos.y;
    
    for (int i = 0; i < MAX_STEPS; i++) {
        float distToCenter = length(pos);
        
        // Check event horizon
        if (distToCenter < u_BlackHoleRadius) {
            hitHorizon = true;
            break;
        }
        
        // Check if we've gone too far
        if (totalDist > MAX_DIST) {
            break;
        }
        
        // Gravitational deflection (simplified Schwarzschild)
        vec3 toCenter = -normalize(pos);
        float gravity = u_BlackHoleRadius * SCHWARZSCHILD_FACTOR / (distToCenter * distToCenter);
        vel = normalize(vel + toCenter * gravity * 0.1);
        
        // Step size (smaller near black hole)
        float stepSize = max(0.01, (distToCenter - u_BlackHoleRadius) * 0.1);
        stepSize = min(stepSize, 0.5);
        
        vec3 newPos = pos + vel * stepSize;
        float newY = newPos.y;
        
        // Check disk intersection (y = 0 plane crossing)
        if (prevY * newY < 0.0 && abs(newY) < u_DiskThickness) {
            // Interpolate to find intersection point
            float t = prevY / (prevY - newY);
            vec3 intersect = pos + vel * stepSize * t;
            float discDist = length(vec2(intersect.x, intersect.z));
            
            vec3 diskColor = sampleDisk(intersect, discDist);
            if (length(diskColor) > 0.0) {
                color += diskColor;
                hitDisk = true;
            }
        }
        
        prevY = newY;
        pos = newPos;
        totalDist += stepSize;
    }
    
    // Background (stars) if we didn't hit the event horizon
    if (!hitHorizon) {
        color += getStars(vel);
    }
    
    // Glow around black hole
    float distToCenter = length(pos);
    if (!hitHorizon && distToCenter < u_BlackHoleRadius * 5.0) {
        float glow = 1.0 - smoothstep(u_BlackHoleRadius, u_BlackHoleRadius * 5.0, distToCenter);
        glow = pow(glow, 3.0);
        color += vec3(1.0, 0.5, 0.2) * glow * u_GlowIntensity * 0.5;
    }
    
    // Photon ring (thin bright ring at ~1.5x Schwarzschild radius)
    float photonRing = smoothstep(0.02, 0.0, abs(distToCenter - u_BlackHoleRadius * 1.5));
    if (!hitHorizon) {
        color += vec3(1.0, 0.9, 0.7) * photonRing * u_GlowIntensity;
    }
    
    // Tone mapping
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, 1.0);
}
