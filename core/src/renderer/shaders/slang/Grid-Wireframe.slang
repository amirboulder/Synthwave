#include "include/Frame_data.slang"
#include "include/Shader_Types.slang"
#include "modules/PhongLighting.slang"

[FillMode("FILL")]
[CullMode("NONE")]
[FrontFace("COUNTER_CLOCKWISE")]
[PrimitiveType("TRIANGLELIST")]
[DepthCompareOp("LESS")]
[BlendState("NONE")]
[DepthFormat("D32_FLOAT_S8_UINT")]
[ColorTarget("R8G8B8A8_UNORM")]
[DepthTest("TRUE")]
[DepthWrite("TRUE")]
[MsaaCapable("TRUE")]
[StencilTest("FALSE")]

layout(set = 1, binding = 0) cbuffer VertexFrameData {
    FrameData vertexFrameData;
};

layout(set = 0, binding = 0) StructuredBuffer<PerMeshData> perMeshData; // per mesh instance

layout(set = 0, binding = 1) StructuredBuffer<PerSubMeshData> perSubmeshData; // per submesh

layout(set = 1, binding = 1) cbuffer BaseDrawIndex {
    uint32_t baseDrawIndex;
};

struct PerMeshData {
    float4x4 worldMatrix;
    float4x4 normalMatrix;
};

struct PerSubMeshData {
    uint32_t instanceOffset;
    uint32_t materialIndex;
    uint32_t _pad[2];
};

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoords : TextureCoords;
    float4 color : COLOR; 
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 worldPos : WORLD_Position;
    float3 normal : NORMAL;
    float2 texCoords : TextureCoords;
    float3 cameraPos;
    nointerpolation uint32_t materialIndex;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input, uint instanceID: SV_InstanceID, uint drawIndex: SV_DrawIndex)
{
    uint32_t subMeshIndex = drawIndex + baseDrawIndex;
    uint32_t globalInstanceID = perSubmeshData[subMeshIndex].instanceOffset + instanceID;
    float4x4 modelMatrix = perMeshData[globalInstanceID].worldMatrix;
    float3x3 normalMat = (float3x3)perMeshData[globalInstanceID].normalMatrix;

    VertexOutput output;

    float3x3 rotmat = float3x3(vertexFrameData.view);
    float3 translationPart = vertexFrameData.view[3].xyz;
    output.cameraPos = -mul(transpose(rotmat), translationPart);

    // World position (before projection) needed for lighting in fragment
    float4 worldPos = mul(modelMatrix, float4(input.position, 1.0));
    output.worldPos = worldPos.xyz;
    output.position = mul(vertexFrameData.viewProjection, worldPos); // clip space

    output.normal = normalize(mul(normalMat, input.normal));

    output.texCoords = input.texCoords;
    output.materialIndex = perSubmeshData[subMeshIndex].materialIndex;

    return output;
}

struct Material {
    uint32_t baseColorTexIndex;
    uint32_t normalTexIndex;
    uint32_t metallicRoughnessTexIndex;
    uint32_t padding;
    float4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float2 padding2;
};


layout(set = 3, binding = 0) cbuffer FragmentFrameData : register(b0, space3) {
    FrameData fragData;
};

layout(set = 3, binding = 1) cbuffer LightingData {
    LightData lightData;
};

// Using Combined Image Sampler, so they will be bound to the same slot
layout(set = 2, binding = 0) SamplerState sampler;
layout(set = 2, binding = 0) Texture2DArray<float4> diffuseTextures;

layout(set = 2, binding = 1) StructuredBuffer<Material> materials;

layout(set = 2, binding = 2) StructuredBuffer<DirectionalLight> directionalLights;
layout(set = 2, binding = 3) StructuredBuffer<DirectionalLight> pointLights;

[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput)
{
    float3 normal = normalize(fragInput.normal);
    float3 viewDir = normalize(fragData.cameraPos - fragInput.worldPos);

    uint32_t baseColorTexIndex = materials[fragInput.materialIndex].baseColorTexIndex;
    float3 tint = diffuseTextures.Sample(sampler, float3(fragInput.texCoords, float(baseColorTexIndex))).rgb;

    // --- Obsidian / glass floor via the shared lighting model ---
    // Dark dielectric: tiny diffuse, weak normal-incidence reflection, strong
    // grazing fresnel rim that picks up the procedural sky.
    Surface surface;
    surface.worldPos  = fragInput.worldPos;
    surface.normal    = normal;
    surface.viewDir   = viewDir;
    surface.baseColor = float3(0.015, 0.015, 0.02) * tint; // dark volcanic glass
    surface.roughness = 0.12;                              // glossy
    surface.metallic  = 0.5;                               // dielectric -> f0 0.04

    //Currently unsed
    LightResult lightResult = calculateLighting(surface, lightData, directionalLights);


    // --- Neon grid lines (emissive, composited over the reflective floor) ---
    float2 coord = fragInput.worldPos.xz;
    float2 cameraPosXZ = fragInput.cameraPos.xz;

    float2 gridSize = float2(4.0, 4.0);

    // Distance-based fade so the grid dissolves into the horizon.
    float distFromCamera = length(coord - cameraPosXZ);
    float fadeDistance = 250.0;
    float fadeOut = 1.0 - smoothstep(fadeDistance * 0.4, fadeDistance, distFromCamera);

    float2 scaledCoord = coord / gridSize;

    // Anti-aliased grid lines via the screen-space derivative of the coordinate.
    float2 grid = abs(fract(scaledCoord - 0.4) - 0.5) / fwidth(scaledCoord);
    float lineDist = min(grid.x, grid.y);

    float baseLineIntensity = 1.0 - min(lineDist, 1.0);

    // Softer bloom falloff around each line.
    float bloomIntensity = 1.0 - min(lineDist * 0.35, 1.0);
    bloomIntensity *= fadeOut;
    bloomIntensity = pow(bloomIntensity, 2.0);

    float finalIntensity = max(baseLineIntensity, bloomIntensity * 0.8 * fadeOut);

    // Shaping / anti-alias curve (kept from the original look).
    finalIntensity = pow(finalIntensity, 1.0 / 2.2);

    float3 neon = float3(1.0, 0.341, 1.0) * finalIntensity; // pink-magenta lines

    float3 result = neon;
    return float4(result, 1.0);
}
