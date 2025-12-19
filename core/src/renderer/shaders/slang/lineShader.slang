layout(set = 1, binding = 0) cbuffer FrameData {
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
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

    // Transposing here for now instead of in cpu because we need viewProjection in its original form to be multipied with model matrix in cpu,which does not apply to this mesh
    float4x4 viewProjT = transpose(viewProjection);

    output.position = mul(viewProjT, float4(input.position, 1.0));
    output.color = input.color;

    
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput)
{
    return fragInput.color;
}

