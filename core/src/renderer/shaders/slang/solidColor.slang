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
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    output.position = mul(mvp, float4(input.position, 1.0));
    output.texCoords = input.texCoords;
    output.color = input.color;

    
    return output;
}

layout(set = 3, binding = 0) cbuffer HighlightColor {
    float4 color;
};

[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput)
{
    return color;
;
}

