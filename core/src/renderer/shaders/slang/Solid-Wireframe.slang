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
    float3 worldPos : WORLD_POSITION;
    float3 barycentric : BARYCENTRIC;
    float3 worldNormal : NORMAL;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input, uint vertexID: SV_VertexID, uint instanceIndex: SV_InstanceID)
{
    float4x4 model = transforms[instanceIndex];
    float4x4 mvp = mul(vertexFrameData.viewProjection, model);

    // Derive barycentric from vertex index within triangle
    uint localID = vertexID % 3;
    float3 bary = float3(float(localID == 0), float(localID == 1), float(localID == 2));

    VertexOutput output;
    output.position = mul(mvp, float4(input.position, 1.0));
    output.worldNormal = input.normal;
    output.barycentric = bary;
    return output;
}


float smoothEdge(float d, float w) {
    float edge = w * 0.5;
    if (d > edge) return 0.0;
    float x = d / edge;
    return exp2(-2.0 * x * x); 
}

[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput) : SV_Target
{
    // Hardcoded visual parameters
    float4 wireColor = float4(0.0, 0.0, 1.0, 1.0);
    float4 fillColor = float4(0.0);
    float lineWidth = 1.8;
    //float fillOpacity = 1.0;

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

