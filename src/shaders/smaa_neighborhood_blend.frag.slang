/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "postprocess.inc.slang"

// SMAA Neighborhood Blending
// Ref: Jimenez et al., "SMAA: Enhanced Morphological Antialiasing", EGSR 2012
// https://www.iryoku.com/smaa/
ConstantBuffer<Params> params : register(b0);
Texture2D inputImage          : register(t1);
Texture2D blendBuffer         : register(t2);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    int2 coord = int2(input.position.xy);
    float4 weights = blendBuffer.Load(int3(coord, 0));
    float4 color   = inputImage.Load(int3(coord, 0));

    if (weights.x > 0.0) {
        float4 left  = inputImage.Load(int3(coord + int2(-1, 0), 0));
        float4 right = inputImage.Load(int3(coord + int2( 1, 0), 0));
        color = lerp(color, (left + right) * 0.5, weights.x);
    }

    if (weights.y > 0.0) {
        float4 up   = inputImage.Load(int3(coord + int2(0, -1), 0));
        float4 down = inputImage.Load(int3(coord + int2(0,  1), 0));
        color = lerp(color, (up + down) * 0.5, weights.y);
    }

    return color;
}