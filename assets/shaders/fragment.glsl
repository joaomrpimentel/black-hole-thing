#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec2 u_Resolution;
uniform float u_Time;
uniform float u_BlackHoleRadius;
uniform float u_DiskInnerRadius;
uniform float u_DiskOuterRadius;
uniform float u_DiskThickness;
uniform vec3 u_DiskColor1;
uniform vec3 u_DiskColor2;
uniform float u_GlowIntensity;
uniform float u_DiskPhase;
uniform float u_CameraDistance;
uniform float u_CameraAngle;
uniform sampler3D u_NoiseTexture;
uniform samplerCube u_StarfieldCubemap;

const float PI = 3.14159265359;
const float SCHWARZSCHILD_FACTOR = 3.0;
const int MAX_STEPS = 200;
const float MAX_DIST = 80.0;
const float EPSILON = 0.001;

// ============================================================================
// NOISE FUNCTIONS
// Used for procedural texturing of the accretion disk and nebula background
// ============================================================================

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

vec4 sampleNoise3D(vec3 p) {
    return texture(u_NoiseTexture, p);
}

float turbulenceFromTexture(vec3 p) {
    vec4 n = sampleNoise3D(p);
    float base = n.r * 2.0 - 1.0;
    float oct2 = n.g * 2.0 - 1.0;
    float oct3 = n.b * 2.0 - 1.0;
    
    float value = abs(base) * 0.5 + abs(oct2) * 0.25 + abs(oct3) * 0.125;
    return value;
}

float signedNoiseFromTexture(vec3 p) {
    float n = sampleNoise3D(p).r;
    return n * 2.0 - 1.0;
}

vec3 rotateY(vec3 v, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

vec3 rotateX(vec3 v, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec3(v.x, c * v.y - s * v.z, s * v.y + c * v.z);
}

// ============================================================================
// STARFIELD - Samples pre-rendered cubemap for O(1) performance
// ============================================================================

vec3 getStars(vec3 rd, float lensingAmount, vec3 initialDir, vec3 cameraPos) {
    vec3 toBlackHole = normalize(-cameraPos);
    float distFromCamera = length(cameraPos);
    float angularDistToCenter = acos(clamp(dot(normalize(initialDir), toBlackHole), -1.0, 1.0));
    
    float apparentSize = u_BlackHoleRadius * 3.0 / distFromCamera;
    float innerZone = clamp(apparentSize * 0.8, 0.05, 0.2);   
    float outerZone = clamp(apparentSize * 2.5, 0.15, 0.6);   
    
    float proximityStretch;
    if (angularDistToCenter < innerZone) {
        proximityStretch = 1.0;
    } else if (angularDistToCenter < outerZone) {
        float t = (angularDistToCenter - innerZone) / (outerZone - innerZone);
        proximityStretch = 1.0 - smoothstep(0.0, 1.0, t);
        proximityStretch = pow(proximityStretch, 0.5);  
    } else {
        proximityStretch = 0.0;
    }
    
    float radiusScale = u_BlackHoleRadius * 2.0;
    
    float totalStretch = max(lensingAmount * 2.0, proximityStretch) * radiusScale;
    
    float stretch = 1.0 + totalStretch * 20.0;
    
    vec3 sampleDir = rd;
    
    vec3 radialDir = normalize(toBlackHole - rd * dot(rd, toBlackHole));
    float radialPull = totalStretch * 0.4;
    sampleDir = normalize(sampleDir + radialDir * radialPull);
    
    sampleDir.y /= stretch;
    sampleDir = normalize(sampleDir);
    
    vec3 col = texture(u_StarfieldCubemap, sampleDir).rgb;
    
    col *= 1.0 + totalStretch * 1.5;
    
    return col;
}

// ============================================================================
// ACCRETION DISK - Texture-based noise for performance
// ============================================================================

vec3 sampleDisk(vec3 pos, float distToCenter) {
    if (distToCenter < u_DiskInnerRadius || distToCenter > u_DiskOuterRadius) {
        return vec3(0.0);
    }
    
    float t = (distToCenter - u_DiskInnerRadius) / (u_DiskOuterRadius - u_DiskInnerRadius);
    float angle = atan(pos.z, pos.x);
    
    // ===== TANGENTIAL GAS STREAKS =====
    float rotAngle = angle - u_DiskPhase * 0.2;
    
    float u = rotAngle / (2.0 * PI);
    float v = t; // 0.0 to 1.0
    

    // === Layer 1: Base Flow (The "river") ===
    // Medium radial freq to define lanes, heavily warped
    float warp = sampleNoise3D(vec3(u * 2.0, v * 1.5, u_Time * 0.05)).b * 0.15;
    vec3 baseCoord = vec3(u * 3.0 + warp, v * 4.0 + warp, 0.0);
    float baseFlow = sampleNoise3D(baseCoord).r; // [0,1]
    
    baseFlow = smoothstep(0.2, 0.8, baseFlow);
    
    // === Layer 2: Engraved Streaks (Texture) ===
    // High freq, stretched tangentially
    // These add the "fast gas" look on top of the heavy river
    vec3 streakCoord = vec3(u * 8.0 + warp * 2.0, v * 12.0, u_Time * 0.1);
    float streaks = sampleNoise3D(streakCoord).g;
    
    streaks = pow(streaks, 2.0);
    
    // === Layer 3: Hotspots/Clumps ===
    // Variation in brightness
    float clumps = sampleNoise3D(vec3(u * 4.0, v * 3.0, 5.0)).b;
    
    float noiseVal = baseFlow * 0.6 + streaks * 0.4;
    noiseVal *= (0.7 + 0.3 * clumps);
    float diskNoise = 0.3 + 0.7 * noiseVal;
    
    float edgeFade = smoothstep(0.0, 0.15, t) * smoothstep(1.0, 0.85, t);
    diskNoise = diskNoise * edgeFade + (1.0 - edgeFade) * 0.1; // Darker at edges
    
    // Temperature gradient
    float temperature = pow(1.0 - t, 0.5);
    vec3 baseColor = mix(u_DiskColor2, u_DiskColor1, temperature);
    
    // Relativistic Doppler beaming
    float orbitalSpeed = 0.4 * (1.0 - t * 0.5);
    vec3 tangent = normalize(vec3(-sin(angle), 0.0, cos(angle)));
    vec3 velocity = tangent * orbitalSpeed;
    vec3 viewDir = normalize(vec3(0.0, sin(u_CameraAngle), cos(u_CameraAngle)));
    
    float dopplerFactor = dot(velocity, viewDir);
    
    float beaming = pow(1.0 + dopplerFactor * 2.0, 3.0);
    beaming = clamp(beaming, 0.1, 5.0);
    
    vec3 color = baseColor;
    if (dopplerFactor > 0.0) {
        vec3 blueShift = vec3(0.8, 0.9, 1.0);
        color = mix(color, color * blueShift * 1.5, dopplerFactor * 1.5);
    } else {
        vec3 redShift = vec3(1.2, 0.6, 0.3);
        color = mix(color, color * redShift * 0.7, abs(dopplerFactor) * 1.2);
    }
    
    color *= 0.3 + 0.7 * diskNoise;
    float brightness = (1.0 - t) * (0.2 + 0.8 * diskNoise) * beaming;
    
    return color * brightness * 2.0;
}

// ============================================================================
// MAIN - Ray marching with gravitational lensing
// Traces rays from the camera through curved spacetime around the black hole.
// Uses a simplified Schwarzschild metric approximation for light bending.
// ============================================================================

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * u_Resolution) / min(u_Resolution.x, u_Resolution.y);
    
    vec3 ro = rotateX(vec3(0.0, 0.0, u_CameraDistance), u_CameraAngle);
    
    vec3 forward = normalize(-ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);
    vec3 rd = normalize(forward + uv.x * right + uv.y * up);
    
    vec3 pos = ro;
    vec3 vel = rd;
    vec3 initialDir = rd;
    
    vec3 color = vec3(0.0);
    float bloomMask = 0.0;
    float totalDist = 0.0;
    bool hitDisk = false;
    bool hitHorizon = false;
    float accumulatedLensing = 0.0;
    float prevY = pos.y;
    
    float photonSphere = u_BlackHoleRadius * 1.5;

    // Bounding Sphere Check
    // If the ray doesn't pass near the black hole system, skip the expensive integration.
    float boundRadius = max(u_DiskOuterRadius * 1.5, u_BlackHoleRadius * 15.0);
    float b = dot(ro, rd);
    float c = dot(ro, ro) - boundRadius * boundRadius;
    float h = b*b - c;
    
    // If ray misses the bounding sphere and we are outside it
    if (h < 0.0 && c > 0.0) {
       // Just render stars with no lensing (or minimal)
       vec3 stars = getStars(rd, 0.0, initialDir, ro);
       FragColor = vec4(stars, 0.0);
       return;
    }

    
    for (int i = 0; i < MAX_STEPS; i++) {
        float distToCenter = length(pos);
        
        if (distToCenter < u_BlackHoleRadius) {
            hitHorizon = true;
            break;
        }
        
        if (totalDist > MAX_DIST) break;
        
        if (distToCenter > u_DiskOuterRadius * 2.5 && dot(vel, pos) > 0.0) {
            break;
        }
        
        vec3 toCenter = -normalize(pos);
        float gravity = u_BlackHoleRadius * SCHWARZSCHILD_FACTOR / (distToCenter * distToCenter);
        
        vec3 oldVel = vel;
        vel = normalize(vel + toCenter * gravity * 0.15);
        accumulatedLensing += 1.0 - dot(oldVel, vel);
        
        float stepSize = clamp((distToCenter - u_BlackHoleRadius) * 0.08, 0.005, 0.4);
        
        vec3 newPos = pos + vel * stepSize;
        float newY = newPos.y;
        
        if (prevY * newY < 0.0 && abs(newY) < u_DiskThickness) {
            float interpT = prevY / (prevY - newY);
            vec3 intersect = pos + vel * stepSize * interpT;
            float discDist = length(vec2(intersect.x, intersect.z));
            
            vec3 diskColor = sampleDisk(intersect, discDist);
            if (length(diskColor) > 0.0) {
                color += diskColor;
                bloomMask = 1.0;
                hitDisk = true;
            }
        }
        
        prevY = newY;
        pos = newPos;
        totalDist += stepSize;
    }
    
    float lensingAmount = clamp(accumulatedLensing * 10.0, 0.0, 1.0);
    
    if (!hitHorizon) {
        color += getStars(vel, lensingAmount, initialDir, ro);
    }
    
    float angularDist = acos(clamp(dot(normalize(rd), normalize(-ro)), -1.0, 1.0));
    float glowRadius = atan(u_BlackHoleRadius * 10.0 / u_CameraDistance);
    
    if (angularDist < glowRadius && !hitHorizon) {
        float glow = 1.0 - smoothstep(0.0, glowRadius, angularDist);
        glow = pow(glow, 1.5);
        color += vec3(1.0, 0.5, 0.2) * glow * u_GlowIntensity;
        bloomMask = max(bloomMask, glow);
    }
    
    float photonRingRadius = atan(u_BlackHoleRadius * 1.5 / u_CameraDistance);
    float photonRing = smoothstep(0.03, 0.0, abs(angularDist - photonRingRadius));
    if (!hitHorizon) {
        color += vec3(1.0, 0.9, 0.7) * photonRing * u_GlowIntensity;
        bloomMask = max(bloomMask, photonRing);
    }
    
    FragColor = vec4(color, bloomMask);
}
