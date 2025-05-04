/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct VertexInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : UV;
    float3 tangent  : TANGENT;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float2 uv       : TEXCOORD2;
    float3 tangent  : TEXCOORD3;
    float3 bitangent: TEXCOORD4;
};