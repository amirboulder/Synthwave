layout(set = 1, binding = 0) cbuffer FrameData {
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
};

layout(set = 1, binding = 1) cbuffer PerModel {
    float4x4 model;
    float4x4 mvp;
};



struct VertexInput
{
    float3 position : POSITION;
};

struct VertexOutput
{
    float4 position : SV_Position;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    output.position = mul(mvp, float4(input.position, 1.0));

    return output;
}


[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput)
{
   
    return float4(1,1,1,1);
}
