layout(set = 0, binding = 0) SamplerState samplerState;
layout(set = 0, binding = 0) Texture2D<float4> stencilTexture; // D32_FLOAT_S8_UINT

layout(set = 2, binding = 0) cbuffer OutlineParams {
    float4 outlineColor;
    float thickness;
};


layout(set = 1, binding = 0, rgba8) RWTexture2D<float4> outlineOutput;

[numthreads(8, 8, 1)]
[shader("compute")]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // Get texture dimensions
    uint2 texSize;
    stencilTexture.GetDimensions(texSize.x, texSize.y);
    
    uint2 pixelCoord = dispatchThreadID.xy;
    
    // Bounds check
    if (pixelCoord.x >= texSize.x || pixelCoord.y >= texSize.y) {
        return;
    }
    
    float2 uv = (float2(pixelCoord) + 0.5) / float2(texSize);
    float2 texelSize = 1.0 / float2(texSize);
    
    // Sample center stencil value
    // Stencil is in the GREEN channel (.g) for D32_FLOAT_S8_UINT format
    float centerStencil = stencilTexture.SampleLevel(samplerState, uv, 0).g;
    
    // Edge detection
    float edgeStrength = 0.0;
    
    // Sample in a circle or grid pattern
    const int samples = 8;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            
            float2 offset = float2(x, y) * texelSize * thickness;
            float stencilValue = stencilTexture.SampleLevel(samplerState, uv + offset, 0).g;
            
            // Edge where stencil values differ
            edgeStrength += abs(stencilValue - centerStencil);
        }
    }
    
    // Normalize
    edgeStrength /= float(samples);
    
    // Threshold to create sharp outline
    float outline = step(0.01, edgeStrength);
    
    // Write result
    outlineOutput[pixelCoord] = float4(outlineColor.rgb, outline);
}