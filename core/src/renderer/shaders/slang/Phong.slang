#include "include/Frame_data.slang"
#include "include/Shader_Types.slang"
#include "modules/PhongLighting.slang"

[FillMode("FILL")]
[CullMode("BACK")]
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

layout(set = 0, binding = 0) StructuredBuffer<PerMeshData> perMeshData;     // per mesh instance

layout(set = 0, binding = 1) StructuredBuffer<PerSubMeshData> perSubmeshData;//per submesh

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
    float4 color : COLOR; // probably gonna deprecate for most meshes except some generated ones
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 worldPos : WORLD_Position;
    float3 normal : NORMAL;
    float2 texCoords : TextureCoords;
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

    // World position (before projection) ù needed for lighting in fragment
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
layout(set = 2, binding = 0) SamplerState sampler0;
layout(set = 2, binding = 0) Texture2DArray<float4> diffuseTextures;

layout(set = 2, binding = 1) SamplerState sampler1;
layout(set = 2, binding = 1) Texture2DArray<float4> metallicRoughnessTextures;

layout(set = 2, binding = 2) SamplerState sampler2;
layout(set = 2, binding = 2) Texture2DArray<float4> normalTextures;

layout(set = 2, binding = 3) StructuredBuffer<Material> materials;

layout(set = 2, binding = 4) StructuredBuffer<DirectionalLight> directionalLights;
layout(set = 2, binding = 5) StructuredBuffer<DirectionalLight> pointLights;

// cotangentFrame() / applyNormalMap() live in modules/PhongLighting.slang (included above).

[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput)
{
    uint32_t baseColorTexIndex = materials[fragInput.materialIndex].baseColorTexIndex;
    uint32_t metallicRoughnessTexIndex = materials[fragInput.materialIndex].metallicRoughnessTexIndex;
    uint32_t normalTexIndex = materials[fragInput.materialIndex].normalTexIndex;

    float4 baseColorFactor = materials[fragInput.materialIndex].baseColorFactor;
    float metallicFactor = materials[fragInput.materialIndex].metallicFactor;
    float roughnessFactor = materials[fragInput.materialIndex].roughnessFactor;

    float4 baseColor = diffuseTextures.Sample(sampler0, float3(fragInput.texCoords, float(baseColorTexIndex)));
    baseColor *= baseColorFactor;

    float metallic = metallicRoughnessTextures.Sample(sampler1, float3(fragInput.texCoords, float(metallicRoughnessTexIndex))).b; // Blue (B) Channel: Metallic map
    metallic *= metallicFactor;

    float roughness = metallicRoughnessTextures.Sample(sampler1, float3(fragInput.texCoords, float(metallicRoughnessTexIndex))).g; // Green (G) Channel: Roughness map
    roughness *= roughnessFactor;

    // Tangent-space normal from the normal map (flat (0,0,1) for the default texture).
    float3 sampledNormal = normalTextures.Sample(sampler2, float3(fragInput.texCoords, float(normalTexIndex))).xyz;

    float3 geometricNormal = normalize(fragInput.normal);
    float3 worldNormal = applyNormalMap(geometricNormal, fragInput.worldPos, fragInput.texCoords, sampledNormal);

    float3 viewDir = normalize(fragData.cameraPos - fragInput.worldPos);

    Surface surface;
    surface.worldPos = fragInput.worldPos;
    surface.normal = worldNormal;
    surface.viewDir = viewDir;
    surface.baseColor = baseColor.xyz;
    surface.metallic = metallic;
    surface.roughness = roughness; 

    LightResult lightResult;
    lightResult = calculateLighting(surface, lightData, directionalLights);

    float3 lighting =  lightResult.diffuse + lightResult.specular; // ambinet already accounted for +Frensel ?
    float4 finalColor = float4(lighting, baseColor.a);
    return finalColor;

}
