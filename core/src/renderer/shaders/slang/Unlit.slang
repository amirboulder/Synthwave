#include "include/Shader_Types.slang"
#include "include/Frame_data.slang"

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

layout(set = 0, binding = 0) StructuredBuffer<float4x4> transforms;

// Using Combined Image Sampler,
layout(set = 2, binding = 0) SamplerState sampler;
layout(set = 2, binding = 0) Texture2D<float4> diffuseTexture;

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
    float2 texCoords : TextureCoords;
    float4 color : COLOR;

};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input, uint instanceIndex: SV_InstanceID)
{
    float4x4 model = transforms[instanceIndex];
    float4x4 mvp = mul(vertexFrameData.viewProjection, model);

    VertexOutput output;

    output.position = mul(mvp, float4(input.position, 1.0));
    output.texCoords = input.texCoords;
    output.color = input.color;

    
    return output;
}


[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput)
{
    float4 textureColor = diffuseTexture.Sample(sampler, fragInput.texCoords);

    return textureColor;
}

