
#include "skinning.h"

//--------------------------------------------------------------------------------------
//  For VS layout
//--------------------------------------------------------------------------------------

#ifdef ID_POSITION
    layout (location = ID_POSITION) in vec3 a_Position;
#endif

#ifdef ID_COLOR_0
    layout (location = ID_COLOR_0) in  vec3 a_Color0;
#endif

#ifdef ID_TEXCOORD_0
    layout (location = ID_TEXCOORD_0) in  vec2 a_UV0;
#endif

#ifdef ID_TEXCOORD_1
    layout (location = ID_TEXCOORD_1) in  vec2 a_UV1;
#endif

#ifdef ID_NORMAL
    layout (location = ID_NORMAL) in  vec3 a_Normal;
#endif

#ifdef ID_TANGENT
    layout (location = ID_TANGENT) in vec4 a_Tangent;
#endif

#ifdef ID_WEIGHTS_0
    layout (location = ID_WEIGHTS_0) in  vec4 a_Weights0;
#endif

#ifdef ID_WEIGHTS_1
    layout (location = ID_WEIGHTS_1) in  vec4 a_Weights1;
#endif

#ifdef ID_JOINTS_0
    layout (location = ID_JOINTS_0) in  uvec4 a_Joints0;
#endif

#ifdef ID_JOINTS_1
    layout (location = ID_JOINTS_1) in  uvec4 a_Joints1;
#endif

layout (location = 0) out VS2PS Output;

void gltfVertexFactory()
{
#ifdef ID_WEIGHTS_0
    mat4 skinningMatrix;
    skinningMatrix  = GetCurrentSkinningMatrix(a_Weights0, a_Joints0);
#ifdef ID_WEIGHTS_1
    skinningMatrix += GetCurrentSkinningMatrix(a_Weights1, a_Joints1);
#endif
#else
    mat4 skinningMatrix =
    {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    };
#endif

	mat4 transMatrix = GetWorldMatrix() * skinningMatrix;
	vec4 pos = transMatrix * vec4(a_Position,1);
	Output.WorldPos = vec3(pos.xyz) / pos.w;
	gl_Position = GetCameraViewProj() * pos; // needs w for proper perspective correction

#ifdef HAS_MOTION_VECTORS
	Output.CurrPosition = gl_Position; // current's frame vertex position 

	mat4 prevTransMatrix = GetPrevWorldMatrix() * skinningMatrix;
	vec3 worldPrevPos = (prevTransMatrix * vec4(a_Position, 1)).xyz;
	Output.PrevPosition = GetPrevCameraViewProj() * vec4(worldPrevPos, 1);
#endif

#ifdef ID_NORMAL
	Output.Normal = normalize(vec3(transMatrix * vec4(a_Normal.xyz, 0.0)));
#endif

#ifdef ID_TANGENT
	Output.Tangent = normalize(vec3(transMatrix * vec4(a_Tangent.xyz, 0.0)));
	Output.Binormal = cross(Output.Normal, Output.Tangent) * a_Tangent.w;
#endif

#ifdef ID_COLOR_0
	Output.Color0 = a_Color0;
#endif

#ifdef ID_TEXCOORD_0
	Output.UV0 = a_UV0;
#endif

#ifdef ID_TEXCOORD_1
	Output.UV1 = a_UV1;
#endif  
}

