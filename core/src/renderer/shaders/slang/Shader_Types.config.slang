
// Used to tell the program how this shader can be used

// NOTE: Attribute is a keyword

//Using strings here instead of enums so the shader metadata file is more readable and easier to parse.

[__AttributeUsage(_AttributeTargets.Var)]
struct FillModeAttribute { String value = "FILL"; }; // FILL or LINE

[__AttributeUsage(_AttributeTargets.Var)]
struct CullModeAttribute { String value = "BACK"; }; //BACK or FRONT

[__AttributeUsage(_AttributeTargets.Var)]
struct FrontFaceAttribute { String value = "COUNTER_CLOCKWISE"; }; // CLOCKWISE or COUNTER_CLOCKWISE

[__AttributeUsage(_AttributeTargets.Var)]
struct PrimitiveTypeAttribute { String value = "TRIANGLELIST"; }; // TRIANGLELIST, LINELIST, TRIANGLESTRIP, LINESTRIP, POINTLIST

[__AttributeUsage(_AttributeTargets.Var)]
struct DepthCompareOpAttribute { String value = "LESS"; }; // NEVER, LESS, EQUAL, LESS_OR_EQUAL, GREATER, NOT_EQUAL, GREATER_OR_EQUAL, ALWAYS

[__AttributeUsage(_AttributeTargets.Var)]
struct BlendStateAttribute { String value = "NONE"; }; // ALPHA, ADDITIVE , NONE Keeping it simple for now may add more options later

//Depth Stencil format
[__AttributeUsage(_AttributeTargets.Var)]
struct DepthFormatAttribute { String value = "D32_FLOAT_S8_UINT"; }; // D32_FLOAT_S8_UINT , D32_FLOAT

//The texture format of the color target
//Add one for each color target
[__AttributeUsage(_AttributeTargets.Var)]
struct ColorTargetAttribute { String value = "R8G8B8A8_UNORM"; }; // R8G8B8A8_UNORM, B8G8R8A8_UNORM, R32_UINT, R16G16B16A16_FLOAT

[__AttributeUsage(_AttributeTargets.Var)]
struct DepthTestAttribute { String value = "TRUE"; }; //TRUE, FALSE

[__AttributeUsage(_AttributeTargets.Var)]
struct DepthWriteAttribute { String value = "TRUE"; }; // TRUE, FALSE

//Determines if the shader can be multisampled
//For example EntID outoups uint32 so it cannot
[__AttributeUsage(_AttributeTargets.Var)]
struct MsaaCapableAttribute { String value = "TRUE"; }; // TRUE, FALSE

[__AttributeUsage(_AttributeTargets.Var)]
struct StencilTestAttribute { String value = "FALSE"; }; // TRUE, FALSE