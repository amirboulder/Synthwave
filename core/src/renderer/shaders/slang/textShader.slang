// This is correct SDL_gpu docs say to do it like this!
layout(set = 1, binding = 0) cbuffer PerModel {
    float4x4 mvp;
};

// This is correct SDL_gpu docs say to do it like this!
layout(set = 2, binding = 0) SamplerState sampler;
layout(set = 2, binding = 0) Texture2D<float4> diffuseTexture;

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR0;
    float2 texCoords : TEXCOORD0;
    
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 texCoords : TEXCOORD0;
    
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


[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput) : SV_Target
{
    float4 texColor = diffuseTexture.Sample(sampler, fragInput.texCoords);

    return texColor * fragInput.color;
}