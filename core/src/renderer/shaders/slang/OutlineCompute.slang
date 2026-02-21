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

    // Load center entity ID directly (no UV needed, integer coords)
    uint centerEntityID = entIdTexture.Load(int3(pixelCoord, 0)).r;

    // Only process pixels belonging to the selected entity
    if (centerEntityID != entId) {
        return;
    }

    // Edge detection - check if any neighbor has different entity ID
    bool isEdge = false;
    int radius = int(thickness);

    for (int x = -radius; x <= radius && !isEdge; x++) {
        for (int y = -radius; y <= radius && !isEdge; y++) {
            if (x == 0 && y == 0) continue;

            int2 neighborCoord = int2(pixelCoord) + int2(x, y);

            // Clamp to texture bounds
            neighborCoord = clamp(neighborCoord, int2(0, 0), int2(screenWidth - 1, screenHeight - 1));

            uint neighborEntityID = entIdTexture.Load(int3(neighborCoord, 0)).r;

            if (neighborEntityID != entId) {
                isEdge = true;
            }
        }
    }

    if (isEdge) {
        outlineOutput[pixelCoord] = outlineColor;
    }
}