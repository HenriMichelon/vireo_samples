/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct VertexOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct Params {
    uint2 imageSize;
    float time;
};
