layout(set = 1, binding = 0) cbuffer FrameData {
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
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
    float3 worldPos : TEXCOORD0;
    float3 cameraPos; 
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input, uint instanceIndex: SV_InstanceID)
{
    float4x4 model = transforms[instanceIndex];
    float4x4 mvp = mul(viewProjection, model);

    VertexOutput output;

    float3x3 rotmat = float3x3(view);
    float3 translationPart = view[3].xyz;

    output.cameraPos = -mul(transpose(rotmat), translationPart);

    output.position = mul(mvp, float4(input.position, 1.0));
    output.worldPos = input.position; 


   
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input)
{
    float2 coord = input.worldPos.xz;
    float2 cameraPosXZ = input.cameraPos.xz;

    float2 gridSize = float2(4.0, 4.0);

    // Calculate distance from camera position for fade-out effect
    float distFromCamera = length(coord - cameraPosXZ);

    // Create fade-out factor (1.0 at camera position, 0.0 at fadeDistance and beyond)
    float fadeDistance = 250.0; // Adjust this value to control fade distance
    float fadeOut = 1.0 - smoothstep(fadeDistance * 0.4, fadeDistance, distFromCamera);

    float2 scaledCoord = coord / gridSize;

    // grid calculation
    float2 grid = abs(fract(scaledCoord - 0.4) - 0.5) / fwidth(scaledCoord);
    float line = min(grid.x, grid.y);

    float baseLineIntensity = 1.0 - min(line, 1.0);

    // Create bloom effect - softer falloff from the line
    float bloomIntensity = 1.0 - min(line * 0.35, 1.0);

    bloomIntensity *= fadeOut;

    bloomIntensity = pow(bloomIntensity, 2.0); // Control bloom shape

    // Combine base line and bloom
    float finalIntensity = max(baseLineIntensity, bloomIntensity * 0.8 * fadeOut);

    // finalIntensity *= fadeOut / 2;

    // Apply gamma correction
    // This seems to be doing the antialiasing
    finalIntensity = pow(finalIntensity, 1.0 / 2.2);

    float red = 0;
    float green = 0;
    float blue = 0;

    red = 1 * finalIntensity;
    green = 0.341 * finalIntensity;
    blue = 1 * finalIntensity;

    return float4(red, green, blue, 1.0 );
}

