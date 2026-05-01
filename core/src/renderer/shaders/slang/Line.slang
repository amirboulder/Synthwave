#include "include/Shader_Types.slang"
#include "include/Frame_data.slang"

[FillMode("FILL")]
[CullMode("BACK")]
[FrontFace("COUNTER_CLOCKWISE")]
[PrimitiveType("LINELIST")]
[DepthCompareOp("LESS")]
[BlendState("ALPHA")]
[DepthFormat("D32_FLOAT_S8_UINT")]
[ColorTarget("R8G8B8A8_UNORM")]
[DepthTest("TRUE")]
[DepthWrite("TRUE")]
[MsaaCapable("TRUE")]
[StencilTest("FALSE")]

layout(set = 1, binding = 0) cbuffer VertexFrameData {
    FrameData vertexFrameData;
};

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;

};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    output.position = mul(vertexFrameData.viewProjection, float4(input.position, 1.0));
    output.color = input.color;

    
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput)
{
    return fragInput.color;
}

