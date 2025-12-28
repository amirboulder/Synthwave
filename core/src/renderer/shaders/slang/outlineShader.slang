// Using Combined Image Sampler,
layout(set = 0, binding = 0) SamplerState samplerState;
layout(set = 0, binding = 0) Texture2D<uint> entIdTexture;

layout(set = 1, binding = 0, rgba8) RWTexture2D<float4> outlineOutput;

layout(set = 2, binding = 0) cbuffer OutlineParams {
    float4 outlineColor;
    float thickness;
    uint entId;        
    uint screenWidth;  
    uint screenHeight; 
};


[numthreads(8, 8, 1)]
[shader("compute")]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;

    // Bounds check using passed dimensions
    if (pixelCoord.x >= screenWidth || pixelCoord.y >= screenHeight) {
        return;
    }

    // Calculate UV coordinates
    float2 uv = (float2(pixelCoord) + 0.5) / float2(screenWidth, screenHeight);
    float2 texelSize = 1.0 / float2(screenWidth, screenHeight);

    // Sample center entity ID (stored as float but represents uint)
    float centerID = entIdTexture.SampleLevel(samplerState, uv, 0).r;
    uint centerEntityID = uint(centerID);

    // Only process pixels belonging to the selected entity
    if (centerEntityID != entId) {
        return; // Skip pixels not part of selected entity
    }

    // Edge detection - check if any neighbor has different entity ID
    bool isEdge = false;

    int radius = int(thickness);
    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            if (x == 0 && y == 0) continue;

            float2 offset = float2(x, y) * texelSize;
            float neighborID = entIdTexture.SampleLevel(samplerState, uv + offset, 0).r;

            // Edge detected if neighbor has different entity ID
            if (uint(neighborID) != entId) {
                isEdge = true;
                break;
            }
        }
        if (isEdge) break;
    }

    // Read existing color and blend outline
    float4 existingColor = outlineOutput[pixelCoord];

    if (isEdge) {
        // Write outline color at edges
        outlineOutput[pixelCoord] = outlineColor;
    }
    // Otherwise leave existing color unchanged
}