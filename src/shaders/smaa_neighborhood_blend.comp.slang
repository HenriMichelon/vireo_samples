/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "smaa_common.inc.slang"

// SMAA Neighborhood Blending
// Ref: Jimenez et al., "SMAA: Enhanced Morphological Antialiasing", EGSR 2012
// https://www.iryoku.com/smaa/
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint2 coord = id.xy;
    if (any(coord >= params.imageSize)) return;

    Texture2D inputImage       = textures[INPUT_BUFFER];
    Texture2D weightBufferInput = textures[WEIGHT_BUFFER];
    float4 weights = weightBufferInput[coord];
    float4 color   = inputImage[coord];

    if (weights.x > 0.0) {
        float4 left  = inputImage[coord + int2(-1, 0)];
        float4 right = inputImage[coord + int2( 1, 0)];
        color = lerp(color, (left + right) * 0.5, weights.x);
    }

    if (weights.y > 0.0) {
        float4 up   = inputImage[coord + int2(0, -1)];
        float4 down = inputImage[coord + int2(0,  1)];
        color = lerp(color, (up + down) * 0.5, weights.y);
    }

    outputImage[coord] = color;
}