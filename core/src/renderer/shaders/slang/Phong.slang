#include "Shader_Types.config.slang"
#include "Structs.config.slang"

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
layout(set = 0, binding = 1) StructuredBuffer<float4x4> normalMatrices;


struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoords : TextureCoords;
    float4 color : COLOR; // probably gonna deprecate for most meshes except some generated ones
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 worldPos : WORLD_Position;
    float3 normal : NORMAL;
    float2 texCoords : TextureCoords;
};



[shader("vertex")]
VertexOutput vertexMain(VertexInput input, uint instanceIndex: SV_InstanceID)
{
    float4x4 modelMatrix = transforms[instanceIndex];
    float3x3 normalMat = (float3x3)normalMatrices[instanceIndex];

    VertexOutput output;

    // World position (before projection) — needed for lighting in fragment
    float4 worldPos = mul(modelMatrix, float4(input.position, 1.0));
    output.worldPos = worldPos.xyz;
    output.position = mul(vertexFrameData.viewProjection, worldPos); // clip space

    output.normal = normalize(mul(normalMat, input.normal));

    output.texCoords = input.texCoords;

    
    return output;
}

// Using Combined Image Sampler,
layout(set = 2, binding = 0) SamplerState sampler;
layout(set = 2, binding = 0) Texture2D<float4> diffuseTexture;


layout(set = 3, binding = 0) cbuffer FragmentFrameData : register(b0, space3) {
    FrameData fragData;
};

layout(set = 3, binding = 1) cbuffer LightData {
    float3 ambientColor;
    float ambientIntensity;

    uint numDirectionalLights;
    uint numPointLights;
} lightData;

layout(set = 2, binding = 1) StructuredBuffer<DirectionalLight> directionalLights;


struct DirectionalLight {
    float3 direction;
    float intensity;
    float3 color;
    float _pad;
};

[shader("fragment")]
float4 fragmentMain(VertexOutput fragInput)
{
    // hardcode material values for now
    //TODO add materials
    float shininess = 0.1;
    float specStrength = 0.1;

    // Ambient — base light so nothing is pure black
    float3 ambient = lightData.ambientColor * lightData.ambientIntensity;

    float3 normal = normalize(fragInput.normal); // re-normalize after interpolation
    float3 viewDir = normalize(fragData.cameraPos - fragInput.worldPos);

    float3 totalDiffuse = float3(0.0);
    float3 totalSpecular = float3(0.0);

    // Loop through all Directional Lights
    for (uint i = 0; i < lightData.numDirectionalLights; i++)
    {
        DirectionalLight dirLight = directionalLights[i];

        float3 lightDir = normalize(dirLight.direction); // normalize(lightPos - fragInput.worldPos);

        // Diffuse — how directly the surface faces the light
        float diff = max(dot(normal, lightDir), 0.0);
        float3 diffuse = diff * dirLight.color * dirLight.intensity;

        totalDiffuse += diffuse;

        // Specular — shiny highlight based on view angle
        float3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        float3 specular = specStrength * spec * dirLight.color * dirLight.intensity;

        totalSpecular += specular;
    }


    //TODO point light loop

    float4 textureColor = diffuseTexture.Sample(sampler, fragInput.texCoords);

    // Combine: texture color is scaled by the lighting result
    float3 lighting = ambient + totalDiffuse + totalSpecular;
    float4 finalColor = float4(textureColor.rgb * lighting, textureColor.a);
    return finalColor;

}

