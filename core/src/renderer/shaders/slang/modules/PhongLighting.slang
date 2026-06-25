// =============================================================================
// lighting.hlsl — Blinn-Phong approximation of glTF metallic-roughness PBR
// =============================================================================
struct Surface {
    float3 worldPos;
    float3 normal;
    float3 viewDir;
    float3 baseColor;  // glTF baseColorFactor — albedo for dielectrics, specular tint for metals
    float  roughness;  // perceptual roughness [0,1] — Material.roughnessFactor
    float  metallic;   // 0 = dielectric, 1 = conductor — Material.metallicFactor
};

struct DirectionalLight {
    float3 direction;
    float  intensity;
    float3 color;
    float  _pad;
};

struct LightData {
    float3 ambientColor;
    float  ambientIntensity;
    uint   numDirectionalLights;
    uint   numPointLights;
};

// Diffuse/specular kept separate so callers can composite with environment maps,
// SSR, or any other indirect contribution independently.
struct LightResult {
    float3 diffuse;    // (ambient diffuse + direct diffuse) * diffuseAlbedo
    float3 specular;   // ambient specular + direct specular highlights
    float3 fresnel;    // NdotV Schlick — use to weight cubemap / SSR reflections
    float3 reflectDir; // View reflection vector for cubemap / SSR sampling
};

// -----------------------------------------------------------------------------
// Schlick Fresnel
// -----------------------------------------------------------------------------
// f0  : base reflectance at normal incidence (0.04 for dielectrics, baseColor for metals)
// cosTheta : dot(H, V) for direct lights; dot(N, V) for ambient / environment
float3 schlickFresnel(float3 f0, float cosTheta)
{
    return f0 + (1.0 - f0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// -----------------------------------------------------------------------------
// Roughness → Blinn-Phong shininess
// -----------------------------------------------------------------------------
// FIX 1: use alpha = roughness², then derive shininess from 2/alpha⁴ - 2.
// This mirrors how GGX's NDF peak width relates to alpha, giving a perceptually
// consistent mapping: roughness 0 → very tight highlight, roughness 1 → broad/matte.
float roughnessToShininess(float roughness)
{
    float r     = saturate(roughness);
    float alpha = r * r;                         // perceptual → linear roughness
    float a4    = max(alpha * alpha, 1e-4);      // α⁴, clamped to avoid /0
    return max(2.0 / a4 - 2.0, 1.0);            // always ≥ 1 (Blinn-Phong valid range)
}

// -----------------------------------------------------------------------------
// Cotangent-frame normal mapping (no precomputed tangents required)
// -----------------------------------------------------------------------------
// Reconstructs the TBN from screen-space derivatives of world position and UVs
// (Schueler's cotangent frame). A flat normal map (0,0,1) leaves the geometric
// normal unchanged, so meshes without a normal texture are unaffected.
// Must be called from a fragment / pixel shader (uses ddx / ddy).
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

    // Guard: degenerate UVs (zero area in UV space) collapse T/B to 0,
    // falling back to the geometric normal safely.
    float denom = max(dot(T, T), dot(B, B));
    float invMax = denom > 0.0 ? rsqrt(denom) : 0.0;
    return float3x3(T * invMax, B * invMax, N);
}

float3 applyNormalMap(float3 geometricNormal, float3 worldPos, float2 uv, float3 sampledNormal)
{
    float3 tangentNormal = sampledNormal * 2.0 - 1.0;   // [0,1] → [-1,1]
    float3x3 TBN = cotangentFrame(geometricNormal, worldPos, uv);
    return normalize(mul(tangentNormal, TBN));
}

// -----------------------------------------------------------------------------
// Main lighting function
// -----------------------------------------------------------------------------
LightResult calculateLighting(
    Surface                          surface,
    LightData                        lightData,
    StructuredBuffer<DirectionalLight> directionalLights)
{
    float3 N = surface.normal;
    float3 V = surface.viewDir;

    // --- glTF metallic-roughness material derivation ---
    // Dielectrics: fixed 4% reflectance, full baseColor diffuse.
    // Conductors:  baseColor is the specular tint, zero diffuse.
    float3 f0            = lerp(float3(0.04, 0.04, 0.04), surface.baseColor, surface.metallic);
    float3 diffuseAlbedo = surface.baseColor * (1.0 - surface.metallic);

    float shininess = roughnessToShininess(surface.roughness);

    // --- Ambient / environment Fresnel (NdotV) ---
    // FIX 5 (part a): evaluate Fresnel at the view angle for ambient splitting.
    float  NdotV         = saturate(dot(N, V));
    float3 fresnelNdotV  = schlickFresnel(f0, NdotV);

    // --- Ambient split: diffuse + specular channels ---
    // FIX 5 (part b): metals get zero ambient diffuse (diffuseAlbedo is already 0),
    // but they must receive ambient through the specular channel — otherwise they
    // look like black voids in areas not hit by direct lights.
    float3 ambientRadiance  = lightData.ambientColor * lightData.ambientIntensity;
    float3 ambientDiffuse   = diffuseAlbedo * (1.0 - fresnelNdotV) * ambientRadiance;
    float3 ambientSpecular  = fresnelNdotV  * ambientRadiance;
    // NOTE: ambientSpecular is a rough approximation. For better results, replace
    // with an irradiance / prefiltered-env-map sample multiplied by fresnelNdotV.

    // --- Direct light accumulation ---
    float3 totalDiffuse  = float3(0.0, 0.0, 0.0);
    float3 totalSpecular = float3(0.0, 0.0, 0.0);

    for (uint i = 0; i < lightData.numDirectionalLights; i++)
    {
        DirectionalLight dirLight = directionalLights[i];

        float3 L       = normalize(-dirLight.direction);
        float3 radiance = dirLight.color * dirLight.intensity;

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0)
            continue;   // back-facing — skip both diffuse and specular

        // FIX 4: Blinn-Phong half-vector lobe (NdotH) replaces Phong dot(V,R).
        // Blinn-Phong's lobe shape is a much closer match to GGX than Phong's,
        // and avoids the hard darkening artefact when the reflection swings behind V.
        float3 H    = normalize(V + L);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = saturate(dot(H, V));

        // FIX 2: per-light Fresnel evaluated at the half-vector angle.
        // Using HdotV (rather than NdotV) is the correct Schlick argument for
        // direct specular; it makes highlights brighten at grazing view angles.
        float3 F = schlickFresnel(f0, HdotV);

        // FIX 3: energy conservation — the fraction reflected (F) cannot also diffuse.
        // kD additionally zeroes out for metals (they have no diffuse term).
        float3 kD = (1.0 - F) * (1.0 - surface.metallic);
        totalDiffuse += kD * NdotL * radiance;

        // Specular highlight — F carries f0 weighting, so no separate f0 multiply.
        // NdotL gate prevents highlights leaking onto back-facing fragments.
        float  spec = pow(NdotH, shininess) * NdotL;
        totalSpecular += F * spec * radiance;
    }

    // TODO: point light loop (same pattern — compute L per fragment, apply distance attenuation)

    // --- Compose result ---
    LightResult result;
    result.diffuse    = ambientDiffuse  + totalDiffuse  * diffuseAlbedo;
    result.specular   = ambientSpecular + totalSpecular;
    result.fresnel    = fresnelNdotV;                 // expose for env-map / SSR weighting
    result.reflectDir = reflect(-V, N);               // for cubemap / SSR sampling

    return result;
}
