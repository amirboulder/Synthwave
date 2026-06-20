struct Surface {
    float3 worldPos;
    float3 normal;
    float3 viewDir;
    float3 baseColor;   // glTF base color -> Material.baseColorFactor (albedo for dielectrics, specular tint for metals)
    float  roughness;   // perceptual roughness [0,1] -> Material.roughnessFactor
    float  metallic;    // 0 = dielectric (glass/obsidian), 1 = metal -> Material.metallicFactor
};

struct DirectionalLight {
    float3 direction;
    float intensity;
    float3 color;
    float _pad;
};

struct LightData {
    float3 ambientColor;
    float ambientIntensity;

    uint numDirectionalLights;
    uint numPointLights;
};

// Light contributions kept SEPARATE so each shader can compose them per-material.
// Diffuse/specular already include the surface's reflectance (derived from metallic),
// so a black obsidian (baseColor ~0, metallic 0) naturally yields ~0 diffuse + f0 specular.
struct LightResult {
    float3 diffuse;    // diffuseAlbedo * (ambient + Σ NdotL*radiance)
    float3 specular;   // Σ f0 * spec * radiance  (direct specular highlights)
    float3 fresnel;    // N·V Schlick term, for weighting environment reflections
    float3 reflectDir; // view reflection vector for sampling a cubemap / SSR
};

// Perceptual roughness [0,1] -> Phong specular exponent.
// roughness 0 -> 1024 (tight, glassy), roughness 1 -> 1 (broad, matte).
float roughnessToShininess(float roughness) {
    float r = saturate(roughness);
    return pow(2.0, (1.0 - r) * 10.0);
}

// --- Normal mapping without precomputed tangents ---
// Our Vertex layout (position/normal/texCoords/color) has no TANGENT attribute, so we
// reconstruct the tangent basis per-fragment from screen-space derivatives of world
// position and UVs (Christian Schueler's cotangent frame). A flat normal map (0,0,1)
// leaves the geometric normal unchanged, so default materials are unaffected.
// NOTE: must be called from a fragment shader (uses ddx/ddy).
// If a TANGENT vertex attribute is added later, swap the body to build the TBN from
// the interpolated tangent and every call site stays the same.
float3x3 cotangentFrame(float3 N, float3 worldPos, float2 uv)
{
    float3 dPosX = ddx(worldPos);
    float3 dPosY = ddy(worldPos);
    float2 dUvX  = ddx(uv);
    float2 dUvY  = ddy(uv);

    float3 dPosYperp = cross(dPosY, N);
    float3 dPosXperp = cross(N, dPosX);

    float3 T = dPosYperp * dUvX.x + dPosXperp * dUvY.x;
    float3 B = dPosYperp * dUvX.y + dPosXperp * dUvY.y;

    // Guard against degenerate UVs (zero-area in UV space); rsqrt(0) would yield Inf/NaN.
    // invMax = 0 collapses T/B to zero so the frame falls back to the geometric normal.
    float denom = max(dot(T, T), dot(B, B));
    float invMax = denom > 0.0 ? rsqrt(denom) : 0.0;
    return float3x3(T * invMax, B * invMax, N);
}

float3 applyNormalMap(float3 geometricNormal, float3 worldPos, float2 uv, float3 sampledNormal)
{
    // Decode from [0,1] texture space to [-1,1] tangent space.
    float3 tangentNormal = sampledNormal * 2.0 - 1.0;
    float3x3 TBN = cotangentFrame(geometricNormal, worldPos, uv);
    return normalize(mul(tangentNormal, TBN));
}

LightResult calculateLighting(
    Surface surface,
    LightData lightData,
    StructuredBuffer<DirectionalLight> directionalLights) {

    float3 N = surface.normal;
    float3 V = surface.viewDir;

    // Metallic-roughness derivation (glTF): dielectrics get a fixed 0.04 reflectance,
    // metals use baseColor as their specular tint and have no diffuse.
    float3 f0           = lerp(float3(0.04), surface.baseColor, surface.metallic);
    float3 diffuseAlbedo = surface.baseColor * (1.0 - surface.metallic);

    float shininess = roughnessToShininess(surface.roughness);

    // View-based Fresnel (Schlick) — how reflective the surface is at grazing angles.
    // This is what gives the glass/obsidian look; exposed for the caller to weight reflections.
    float NdotV = saturate(dot(N, V));
    float3 fresnel = f0 + (1.0 - f0) * pow(1.0 - NdotV, 5.0);

    float3 ambient = lightData.ambientColor * lightData.ambientIntensity;

    float3 totalDiffuse = float3(0.0);
    float3 totalSpecular = float3(0.0);

    // Directional lights
    for (uint i = 0; i < lightData.numDirectionalLights; i++)
    {
        DirectionalLight dirLight = directionalLights[i];

        float3 lightDir = normalize(dirLight.direction);
        float3 radiance = dirLight.color * dirLight.intensity;

        // Diffuse irradiance (un-tinted); albedo applied below
        float NdotL = max(dot(N, lightDir), 0.0);
        totalDiffuse += NdotL * radiance;

        // Specular — tight highlight scaled by base reflectance f0 (no magic constant).
        // Multiplying by NdotL prevents highlights leaking onto back-facing fragments.
        float3 reflectDir = reflect(-lightDir, N);
        float spec = pow(max(dot(V, reflectDir), 0.0), shininess) * NdotL;
        totalSpecular += f0 * spec * radiance;
    }

    // TODO point light loop

    LightResult lightResult;
    lightResult.diffuse    = diffuseAlbedo * (ambient + totalDiffuse);
    lightResult.specular   = totalSpecular;
    lightResult.fresnel    = fresnel;
    lightResult.reflectDir = reflect(-V, N);

    return lightResult;
}

