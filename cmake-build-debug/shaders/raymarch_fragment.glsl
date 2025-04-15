#version 330 core
out vec4 FragColor;

in vec2 TexCoords; // UV coordinates from vertex shader (0.0 to 1.0)

// --- Uniforms ---
uniform float u_time;      // Time for animation (clouds will move)
uniform vec3 u_camPos;     // Camera position in world space
uniform mat4 u_invViewMatrix; // Inverse of the view matrix
uniform mat4 u_invProjMatrix; // Inverse of the projection matrix
uniform vec3 u_lightDir;     // Direction TO the light source
uniform vec3 u_lightColor;
uniform float u_ambientStrength;
// Terrain
uniform float u_terrain_base_freq;
uniform float u_terrain_base_amp;
uniform float u_terrain_persistence;
uniform float u_terrain_flatten_power;
uniform float u_terrain_final_scale;
uniform int   u_terrain_octaves;
// Clouds
uniform float u_cloud_base_height;
uniform float u_cloud_thickness;
uniform float u_cloud_noise_scale;
uniform float u_cloud_coverage_min;
uniform float u_cloud_coverage_max;
uniform float u_cloud_density_factor;
// --- End Uniforms ---

// Constants...
const int MAX_TERRAIN_STEPS = 100;
const float MIN_HIT_DISTANCE = 0.001;
const float MAX_TRACE_DISTANCE = 100.0;
const int MAX_SHADOW_STEPS = 32;
const float SHADOW_MAX_DISTANCE = 50.0;
const float SHADOW_DENSITY_MULTIPLIER = 0.4;
const int MAX_CLOUD_STEPS = 80;
const float CLOUD_STEP_SIZE = 0.8;
const float CLOUD_DENSITY_MULTIPLIER = 1.5;
// const float cloudBaseHeight = 10.0; // Now uniform u_cloud_base_height
// const float cloudThickness = 12.0; // Now uniform u_cloud_thickness

// --- Noise Functions (2D and 3D) ---
float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }
float hash3(vec3 p) { p = fract(p * 0.1031); p += dot(p, p.yzx + 33.33); return fract((p.x + p.y) * p.z); }
float valueNoise(vec2 p) { vec2 i=floor(p); vec2 f=fract(p); vec2 u=f*f*(3.0-2.0*f); return mix(mix(hash(i+vec2(0,0)), hash(i+vec2(1,0)), u.x), mix(hash(i+vec2(0,1)), hash(i+vec2(1,1)), u.x), u.y); }
float valueNoise3D(vec3 p) { vec3 i=floor(p); vec3 f=fract(p); vec3 u=f*f*(3.0-2.0*f); return mix(mix(mix(hash3(i+vec3(0,0,0)), hash3(i+vec3(1,0,0)), u.x), mix(hash3(i+vec3(0,1,0)), hash3(i+vec3(1,1,0)), u.x), u.y), mix(mix(hash3(i+vec3(0,0,1)), hash3(i+vec3(1,0,1)), u.x), mix(hash3(i+vec3(0,1,1)), hash3(i+vec3(1,1,1)), u.x), u.y), u.z); }
// --- FBM Functions ---
float fbm_raw(vec2 p, out float maxAmp) {
    float freq = u_terrain_base_freq; float amp = u_terrain_base_amp; float persistence = u_terrain_persistence; int octaves = u_terrain_octaves;
    float value = 0.0; maxAmp = 0.0;
    for(int i=0; i<octaves; i++) { if (amp < 0.01) break; value += valueNoise(p*freq)*amp; maxAmp += amp; freq *= 2.0; amp *= persistence; }
    return value;
}
float fbm3D(vec3 p, int octaves, float persistence) {
    float freq = 1.0; float amp = 0.5; float value = 0.0;
    for(int i=0; i<octaves; i++) { value += valueNoise3D(p * freq) * amp; freq *= 2.0; amp *= persistence; }
    return value;
}
// --- End Noise ---

// --- Terrain Height & SDF ---
float terrainHeight(vec2 p) {
    float maxPossibleAmplitude; float raw_fbm = fbm_raw(p, maxPossibleAmplitude);
    float normalized_fbm = (maxPossibleAmplitude > 0.0) ? (raw_fbm / maxPossibleAmplitude) : 0.0;
    float flattened_fbm = pow(normalized_fbm, u_terrain_flatten_power);
    return flattened_fbm * u_terrain_final_scale;
}
float mapScene(vec3 p) { return p.y - terrainHeight(p.xz); }
// --- End Terrain ---

// --- Cloud Density Function ---
float mapClouds(vec3 p) {
    float y_norm = (p.y - u_cloud_base_height) / u_cloud_thickness;
    float verticalFalloff = smoothstep(0.0, 0.1, y_norm) * (1.0 - smoothstep(0.9, 1.0, y_norm));
    if (verticalFalloff <= 0.0) return 0.0;
    vec3 noiseSamplePos = p * u_cloud_noise_scale + vec3(u_time * 0.05, 0.0, u_time * 0.02);
    float baseNoise = fbm3D(noiseSamplePos, 4, 0.5);
    float density = smoothstep(u_cloud_coverage_min, u_cloud_coverage_max, baseNoise);
    return density * verticalFalloff * u_cloud_density_factor;
}
// --- End Clouds ---


// --- Calculate Normal ---
vec3 calcNormal(vec3 p) { vec2 e=vec2(0.01,0.0); return normalize(vec3(mapScene(p+e.xyy)-mapScene(p-e.xyy), mapScene(p+e.yxy)-mapScene(p-e.yxy), mapScene(p+e.yyx)-mapScene(p-e.yyx))); }

// --- Terrain Ray Marching ---
float rayMarchTerrain(vec3 ro, vec3 rd, out vec3 hitPos) {
    float t = 0.0;
    for(int i = 0; i < MAX_TERRAIN_STEPS; i++) {
        hitPos = ro + rd * t; float dist = mapScene(hitPos);
        if(dist < MIN_HIT_DISTANCE) { return t; } // Hit
        t += max(dist, MIN_HIT_DISTANCE * 0.5);
        if(t >= MAX_TRACE_DISTANCE) { return -1.0; } // Miss (too far)
    }
    return -1.0; // <<<< ENSURE THIS RETURN IS PRESENT
}

// --- Cloud Shadow Ray Marching ---
float marchShadowRay(vec3 ro, vec3 rd) { // rd should be light direction
    float t = 0.01; float accumulatedDensity = 0.0; float shadowFactor = 1.0;
    for(int i = 0; i < MAX_SHADOW_STEPS; i++) {
        vec3 currentPos = ro + rd * t; float density = mapClouds(currentPos);
        if (density > 0.01) {
            float stepSize = SHADOW_MAX_DISTANCE / float(MAX_SHADOW_STEPS);
            accumulatedDensity += density * stepSize;
            shadowFactor = exp(-accumulatedDensity * SHADOW_DENSITY_MULTIPLIER);
        }
        t += SHADOW_MAX_DISTANCE / float(MAX_SHADOW_STEPS);
        if(t >= SHADOW_MAX_DISTANCE) { break; }
    } return clamp(shadowFactor, 0.0, 1.0);
}

// --- Volumetric Cloud Ray Marching ---
vec4 marchClouds(vec3 ro, vec3 rd) {
    float t = 0.0; vec4 accumulatedColor = vec4(0.0);
    vec3 cloudColor = vec3(1.0);
    for(int i = 0; i < MAX_CLOUD_STEPS; ++i) {
        if (accumulatedColor.a > 0.99) break;
        vec3 currentPos = ro + rd * t;
        if (currentPos.y < u_cloud_base_height - 1.0) break;
        float density = mapClouds(currentPos);
        if (density > 0.01) {
            float lightPhase = pow(max(dot(rd, -u_lightDir), 0.0), 2.0) * 0.5 + 0.5;
            vec3 litCloudColor = cloudColor * lightPhase;
            float alpha = (1.0 - exp(-density * CLOUD_STEP_SIZE * CLOUD_DENSITY_MULTIPLIER));
            vec4 stepColor = vec4(litCloudColor, alpha);
            accumulatedColor.rgb += (1.0 - accumulatedColor.a) * stepColor.rgb * stepColor.a;
            accumulatedColor.a += (1.0 - accumulatedColor.a) * stepColor.a;
        }
        t += CLOUD_STEP_SIZE;
        if (t >= MAX_TRACE_DISTANCE) break;
    } return accumulatedColor;
}


// --- Blinn-Phong Lighting Calculation ---
vec3 calculateLighting(vec3 fragPos, vec3 normal, vec3 viewDir,
vec3 lightDir, vec3 lightColor, float ambientStrength,
vec3 surfaceColor, float shadowFactor)
{
    float diffuseStrength = 0.8; float specularStrength = 0.03; float shininess = 32.0;
    float skyOcclusion = smoothstep(0.0, 0.5, normal.y);
    vec3 ambient = ambientStrength * skyOcclusion * lightColor;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * skyOcclusion * diff * lightColor;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    vec3 result = ambient * surfaceColor + (diffuse * surfaceColor + specular) * shadowFactor;
    return result;
}

// --- Generate Ray Direction ---
vec3 getRayDirection(vec2 uv, vec3 camPos) {
    vec2 ndc = uv * 2.0 - 1.0; vec4 clipPos = vec4(ndc.x, ndc.y, -1.0, 1.0);
    vec4 viewPos = u_invProjMatrix * clipPos; viewPos /= viewPos.w;
    vec4 worldPos = u_invViewMatrix * vec4(viewPos.xyz, 1.0);
    return normalize(worldPos.xyz - camPos);
}

// --- Main Fragment Shader Entry Point ---
void main()
{
    vec2 uv = TexCoords; vec3 rayOrigin = u_camPos;
    vec3 rayDirection = getRayDirection(uv, rayOrigin);

    vec3 hitPosition;
    float distanceTraveled = rayMarchTerrain(rayOrigin, rayDirection, hitPosition);

    vec3 skyColor = vec3(0.5, 0.7, 1.0);
    vec3 finalColor = skyColor;

    if (distanceTraveled > 0.0) { // Hit terrain
        // --- Terrain shading ---
        vec3 normal = calcNormal(hitPosition); float altitude = hitPosition.y; float slope = 1.0 - normal.y;
        vec3 grassColor=vec3(0.2,0.5,0.1); vec3 rockColor=vec3(0.5,0.45,0.4); vec3 dirtColor=vec3(0.6,0.4,0.2); vec3 snowColor=vec3(0.9,0.9,0.95);
        float rockFactor=smoothstep(0.2, 1.0, altitude); vec3 baseColor=mix(grassColor,rockColor,rockFactor);
        float grassSlopeFactor=smoothstep(0.4,0.7,slope); baseColor=mix(baseColor,rockColor,grassSlopeFactor);
        float patchNoiseFreq=1.5; float patchNoise=valueNoise(hitPosition.xz*patchNoiseFreq); float patchBlend=smoothstep(0.3,0.6,patchNoise); float patchSlopeFactor=1.0-smoothstep(0.3,0.6,slope); float patchAltitudeFactor=1.0-smoothstep(0.5, 1.5, altitude);
        patchBlend*=patchSlopeFactor*patchAltitudeFactor*0.5; baseColor=mix(baseColor,dirtColor,patchBlend);
        float snowFactor=smoothstep(1.8, 2.2, altitude); vec3 surfaceColor=mix(baseColor,snowColor,snowFactor);
        // Use uniforms for light properties
        float shadowFactor = marchShadowRay(hitPosition, u_lightDir);
        vec3 viewDir = normalize(u_camPos - hitPosition);
        // Pass uniforms explicitly to lighting function
        finalColor = calculateLighting(hitPosition, normal, viewDir,
        u_lightDir, u_lightColor, u_ambientStrength, // Pass uniforms
        surfaceColor, shadowFactor);
        // --- End Terrain Shading ---

    } else { // Missed terrain -> Render Clouds
        // --- Render Clouds ---
        vec4 cloudColor = marchClouds(rayOrigin, rayDirection);
        finalColor = mix(skyColor, cloudColor.rgb, cloudColor.a);
        // --- End Render Clouds ---
    }

    // --- Fog (Apply AFTER terrain/cloud calculation) ---
    float fogDistance = (distanceTraveled > 0.0) ? distanceTraveled : MAX_TRACE_DISTANCE;
    float fogAmount = smoothstep(10.0, MAX_TRACE_DISTANCE * 0.8, fogDistance);
    finalColor = mix(finalColor, skyColor, fogAmount);

    FragColor = vec4(finalColor, 1.0);
}