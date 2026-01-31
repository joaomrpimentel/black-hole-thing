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
const float SCHWARZSCHILD_FACTOR = 3.0; // Increased for more dramatic light bending
const int MAX_STEPS = 300; // More steps for smoother curves
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

// 3D Simplex Noise for volumetric turbulence
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x * 34.0) + 1.0) * x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v) {
    const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

    vec3 i = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;

    i = mod289(i);
    vec4 p = permute(permute(permute(
        i.z + vec4(0.0, i1.z, i2.z, 1.0))
        + i.y + vec4(0.0, i1.y, i2.y, 1.0))
        + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    float n_ = 0.142857142857;
    vec3 ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);

    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    vec4 norm = taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m * m, vec4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));
}

// 3D Turbulence using Simplex Noise (FBM)
float turbulence3D(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * abs(snoise(p * frequency));
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

// Starfield background with lensing-aware stretching
vec3 getStars(vec3 rd, float lensingAmount, vec3 initialDir, vec3 cameraPos) {
    vec3 col = vec3(0.0);
    
    // Calculate proximity to black hole center based on INITIAL ray direction
    // Stars that initially pointed toward the black hole should stretch, 
    // regardless of where the ray ended up after bending
    vec3 toBlackHole = normalize(-cameraPos); // Direction from camera to black hole
    float angularDistToCenter = acos(clamp(dot(normalize(initialDir), toBlackHole), -1.0, 1.0));
    
    // Stars very close to the event horizon (small angular distance) should stretch a lot
    // Use a tighter threshold for the stretching zone
    float stretchZone = 0.3; // About 17 degrees from center
    float proximityStretch = 1.0 - smoothstep(0.0, stretchZone, angularDistToCenter);
    proximityStretch = pow(proximityStretch, 0.5); // More gradual falloff
    
    // Use lensing amount as the primary stretch factor for rays that bent
    // Combine with proximity for rays near the black hole
    float totalStretch = max(lensingAmount, proximityStretch * 0.5);
    float stretch = 1.0 + totalStretch * 6.0;
    
    // Compute stretch direction: tangent to circles around the black hole center
    // Project ray direction onto spherical coordinates
    float theta = atan(rd.z, rd.x); // azimuthal angle
    float phi = asin(clamp(rd.y, -1.0, 1.0)); // polar angle
    
    // Multiple layers of stars
    for (int i = 0; i < 4; i++) {
        float layerDist = 50.0 + float(i) * 40.0;
        
        // Use spherical coordinates for more accurate lensing
        // Stars should stretch tangentially (perpendicular to radial direction from center)
        vec2 sphereCoord = vec2(theta, phi) * layerDist * 0.5;
        
        // Apply radial stretching: compress in the tangential direction creates apparent elongation
        vec2 stretchedCoord = sphereCoord;
        stretchedCoord.y /= stretch; // Stretch in the phi (vertical) direction
        
        vec2 id = floor(stretchedCoord + float(i) * 7.3);
        float star = hash(id);
        
        if (star > 0.96) {
            float brightness = pow(star - 0.96, 0.4) * 40.0;
            
            // Star position within cell
            vec2 starUV = fract(stretchedCoord + float(i) * 7.3) - 0.5;
            
            // Elongate the star shape based on total stretch amount
            float starSize = 0.03 + totalStretch * 0.25;
            // Stars appear stretched in the tangential direction
            float d = length(starUV * vec2(1.0, stretch * 0.5));
            brightness *= smoothstep(starSize, 0.0, d);
            
            // Star color variation
            vec3 starColor = mix(vec3(1.0, 0.95, 0.9), vec3(0.9, 0.95, 1.0), hash(id + 0.5));
            col += starColor * brightness;
        }
    }
    
    // Subtle nebula
    float nebula = fbm(rd.xy * 3.0 + rd.z * 2.0) * 0.1;
    col += vec3(0.08, 0.04, 0.12) * nebula;
    
    return col;
}

// Sample the accretion disk with 3D turbulence and relativistic beaming
vec3 sampleDisk(vec3 pos, float distToCenter) {
    if (distToCenter < u_DiskInnerRadius || distToCenter > u_DiskOuterRadius) {
        return vec3(0.0);
    }
    
    // Normalized position in disk
    float t = (distToCenter - u_DiskInnerRadius) / (u_DiskOuterRadius - u_DiskInnerRadius);
    
    // Disk angle for animation
    float angle = atan(pos.z, pos.x);
    
    // Fast-flowing gas streaks along orbital direction
    // Use angle-based coordinate to create tangential flow
    float orbitalPhase = angle * 8.0 - u_Time * 2.0; // Fast rotation
    float radialCoord = distToCenter * 3.0;
    
    // Directional noise: stretched along orbital direction
    vec3 flowPos = vec3(
        orbitalPhase,
        radialCoord,
        u_Time * 0.5 + distToCenter * 0.3
    );
    
    // Primary flow turbulence - stretched in flow direction
    float flowNoise = snoise(flowPos * vec3(0.3, 1.0, 1.0));
    flowNoise = 0.5 + 0.5 * flowNoise;
    
    // Add fine detail streaks
    float fineStreaks = snoise(flowPos * vec3(0.5, 2.0, 1.5) + vec3(u_Time * 0.8, 0.0, 0.0));
    fineStreaks = pow(abs(fineStreaks), 0.3);
    
    // Swirling vortices at random intervals
    float vortex = snoise(vec3(angle * 2.0 + u_Time * 0.3, distToCenter * 1.5, u_Time * 0.2));
    vortex = pow(abs(vortex), 2.0) * 0.5;
    
    float diskNoise = flowNoise * 0.6 + fineStreaks * 0.3 + vortex * 0.1;
    
    // Temperature gradient (hotter near center)
    float temperature = 1.0 - t;
    temperature = pow(temperature, 0.5);
    
    // Base color from temperature
    vec3 baseColor = mix(u_DiskColor2, u_DiskColor1, temperature);
    
    // ========================================
    // RELATIVISTIC DOPPLER BEAMING
    // ========================================
    // The disk rotates counter-clockwise when viewed from above.
    // Orbital velocity direction is tangent to the circle at each point.
    // v_orbital = (-sin(angle), 0, cos(angle)) * orbitalSpeed
    
    // Orbital speed (fraction of c, higher near black hole)
    float orbitalSpeed = 0.4 * (1.0 - t * 0.5); // Faster near inner edge
    
    // Tangent direction (perpendicular to radial, in XZ plane)
    vec3 tangent = normalize(vec3(-sin(angle), 0.0, cos(angle)));
    vec3 velocity = tangent * orbitalSpeed;
    
    // View direction from disk point to camera (simplified: assume camera is at +Z)
    // In reality we'd pass camera position, but this approximation works for the effect
    vec3 viewDir = normalize(vec3(0.0, sin(u_CameraAngle), cos(u_CameraAngle)));
    
    // Doppler factor: how much the velocity component is toward/away from camera
    // Positive = approaching (blueshift), Negative = receding (redshift)
    float dopplerFactor = dot(velocity, viewDir);
    
    // Relativistic beaming intensity boost (simplified relativistic formula)
    // I' = I * D^3, where D is the Doppler factor
    // For visual effect, we use a softer power
    float beamingExponent = 3.0;
    float beaming = pow(1.0 + dopplerFactor * 2.0, beamingExponent);
    beaming = clamp(beaming, 0.1, 5.0);
    
    // Color shift: blueshift for approaching, redshift for receding
    vec3 color = baseColor;
    
    // Shift toward blue (higher temperature colors) when approaching
    if (dopplerFactor > 0.0) {
        // Approaching: shift toward white/blue
        vec3 blueShift = vec3(0.8, 0.9, 1.0);
        color = mix(color, color * blueShift * 1.5, dopplerFactor * 1.5);
    } else {
        // Receding: shift toward red/orange
        vec3 redShift = vec3(1.2, 0.6, 0.3);
        color = mix(color, color * redShift * 0.7, abs(dopplerFactor) * 1.2);
    }
    
    // Add turbulent variation to color
    color *= 0.4 + 0.6 * diskNoise;
    
    // Brightness with beaming applied
    float brightness = (1.0 - t) * (0.2 + 0.8 * diskNoise);
    brightness *= beaming;
    
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
    vec3 initialDir = rd;
    
    vec3 color = vec3(0.0);
    float bloomMask = 0.0; // 0 = no bloom (stars), 1 = bloom (disk, glow)
    float totalDist = 0.0;
    bool hitDisk = false;
    bool hitHorizon = false;
    float accumulatedLensing = 0.0; // Track how much the ray bent
    
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
        
        // Gravitational deflection (enhanced Schwarzschild)
        vec3 toCenter = -normalize(pos);
        float gravity = u_BlackHoleRadius * SCHWARZSCHILD_FACTOR / (distToCenter * distToCenter);
        
        // Stronger integration for more dramatic bending
        vec3 oldVel = vel;
        vel = normalize(vel + toCenter * gravity * 0.15);
        
        // Accumulate lensing (angle change)
        accumulatedLensing += 1.0 - dot(oldVel, vel);
        
        // Step size (smaller near black hole for accuracy)
        float stepSize = max(0.005, (distToCenter - u_BlackHoleRadius) * 0.08);
        stepSize = min(stepSize, 0.4);
        
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
                bloomMask = 1.0; // Disk should bloom
                hitDisk = true;
            }
        }
        
        prevY = newY;
        pos = newPos;
        totalDist += stepSize;
    }
    
    // Compute final lensing amount for star stretching
    float lensingAmount = clamp(accumulatedLensing * 10.0, 0.0, 1.0);
    
    // Background (stars) if we didn't hit the event horizon
    if (!hitHorizon) {
        color += getStars(vel, lensingAmount, initialDir, ro);
        // Stars should NOT bloom - bloomMask stays at 0.0 for star-only pixels
    }
    
    // Glow around black hole - based on screen-space proximity, not ray endpoint
    // Calculate angular distance from view center (where black hole is)
    float angularDist = acos(clamp(dot(normalize(rd), normalize(-ro)), -1.0, 1.0));
    // Make glow radius much larger so it's visible - about 10x the black hole angular size
    float glowRadius = atan(u_BlackHoleRadius * 10.0 / u_CameraDistance);
    
    if (angularDist < glowRadius && !hitHorizon) {
        float glow = 1.0 - smoothstep(0.0, glowRadius, angularDist);
        glow = pow(glow, 1.5); // Softer falloff
        color += vec3(1.0, 0.5, 0.2) * glow * u_GlowIntensity;
        bloomMask = max(bloomMask, glow);
    }
    
    // Photon ring (thin bright ring at ~1.5x Schwarzschild radius)
    float photonRingRadius = atan(u_BlackHoleRadius * 1.5 / u_CameraDistance);
    float photonRing = smoothstep(0.03, 0.0, abs(angularDist - photonRingRadius));
    if (!hitHorizon) {
        color += vec3(1.0, 0.9, 0.7) * photonRing * u_GlowIntensity;
        bloomMask = max(bloomMask, photonRing); // Photon ring should bloom
    }
    
    // Output HDR values - tone mapping and gamma done in post-processing
    // Alpha channel stores bloom mask: 1.0 = bloom, 0.0 = no bloom
    FragColor = vec4(color, bloomMask);
}
