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
    float3 barycentric : BARYCENTRIC;
    nointerpolation uint32_t materialIndex;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input, uint vertexID: SV_VertexID, uint instanceID: SV_InstanceID, uint drawIndex: SV_DrawIndex)
{
    uint32_t subMeshIndex = drawIndex + baseDrawIndex;
    uint32_t globalInstanceID = perSubmeshData[subMeshIndex].instanceOffset + instanceID;
    float4x4 modelMatrix = perMeshData[globalInstanceID].worldMatrix;
    float3x3 normalMat = (float3x3)perMeshData[globalInstanceID].normalMatrix;

    // Derive barycentric from vertex index within triangle
    uint localID = vertexID % 3;
    float3 bary = float3(float(localID == 0), float(localID == 1), float(localID == 2));

    VertexOutput output;

    // World position (before projection)  needed for lighting in fragment
    float4 worldPos = mul(modelMatrix, float4(input.position, 1.0));
    output.worldPos = worldPos.xyz;
    output.position = mul(vertexFrameData.viewProjection, worldPos); // clip space

    output.normal = normalize(mul(normalMat, input.normal));

    output.texCoords = input.texCoords;
    output.materialIndex = perSubmeshData[subMeshIndex].materialIndex;
    output.barycentric = bary;
    return output;
}


float smoothEdge(float d, float w) {
    float edge = w * 0.5;
    if (d > edge) return 0.0;
    float x = d / edge;
    return exp2(-2.0 * x * x); 
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

[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput) : SV_Target
{

    // Hardcoded visual parameters
    float4 wireColor = float4(0.0, 0.0, 1.0, 1.0);
    float4 fillColor = float4(0.0);
    float lineWidth = 1.8;
    // float fillOpacity = 1.0;

    float3 bary = fragInput.barycentric;

    // Calculate screen-space derivatives to get actual pixel distances
    float3 dBary = fwidth(bary);

    // Convert barycentric coordinates to actual pixel distances
    float3 distances = bary / dBary;

    // Find minimum distance to any edge
    float minDistance = min(min(distances.x, distances.y), distances.z);

    // Calculate wireframe alpha using smoothing function
    float lineAlpha = smoothEdge(minDistance, lineWidth);

    // If fragment is beyond line width, use fill color only
    if (minDistance > lineWidth * 0.5) {
        return fillColor;
    }
    // Blend line color with fill color near edges
    else {
        // Blend between fill color and line color based on distance
        float4 baseColor = lerp(fillColor, wireColor, lineAlpha);
        return float4(baseColor.rgb, max(fillColor.a, wireColor.a * lineAlpha));
    }
}

