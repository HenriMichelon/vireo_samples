/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "smaa_common.inc.slang"

// SMAA Blend Weight Calculation
// Ref: Jimenez et al., "SMAA: Enhanced Morphological Antialiasing", EGSR 2012
// https://www.iryoku.com/smaa/
ConstantBuffer<Params> params : register(b0);
ConstantBuffer<Data>   data   : register(b1);
Texture2D edgeMap             : register(t2);

int lineSearch(int2 start, int2 dir, int maxSteps, int ch) {
    int steps = 0;
    int2 pos  = start + dir;
    for (int i = 0; i < maxSteps; ++i) {
        if (any(pos < int2(0, 0)) || any(uint2(pos) >= params.imageSize)) break;
        if (edgeMap.Load(int3(pos, 0))[ch] <= 0.0) break;
        pos   += dir;
        steps += 1;
    }
    return steps;
}

float areaFromLength(int lenA, int lenB) {
    int total = lenA + lenB;
    if (total == 0) return 1.0;
    float t = float(lenA) / float(total);
    return 1.0 - 2.0 * abs(t - 0.5);
}

float2 fragmentMain(VertexOutput input) : SV_TARGET {
    int2 coord = int2(input.position.xy);
    float2 edge = edgeMap.Load(int3(coord, 0)).rg;

    float blendV = 0.0;
    float blendH = 0.0;

    if (edge.r > 0.0) {
        int left  = lineSearch(coord, int2(-1,  0), data.blendMaxSteps, 0);
        int right = lineSearch(coord, int2( 1,  0), data.blendMaxSteps, 0);
        blendV = areaFromLength(left, right);
    }

    if (edge.g > 0.0) {
        int up   = lineSearch(coord, int2( 0, -1), data.blendMaxSteps, 1);
        int down = lineSearch(coord, int2( 0,  1), data.blendMaxSteps, 1);
        blendH = areaFromLength(up, down);
    }

    return float2(blendH, blendV);
}