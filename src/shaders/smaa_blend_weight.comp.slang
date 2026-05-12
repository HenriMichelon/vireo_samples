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
int lineSearch(Texture2D edgeMap, int2 start, int2 dir, int maxSteps, int ch) {
    int steps = 0;
    int2 pos  = start + dir;
    for (int i = 0; i < maxSteps; ++i) {
        if (any(pos < int2(0, 0)) || any(uint2(pos) >= params.imageSize)) break;
        if (edgeMap[pos][ch] <= 0.0) break;
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

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint2 coord = id.xy;
    if (any(coord >= params.imageSize)) return;

    Texture2D edgeMap = textures[EDGE_BUFFER];
    float2 edge = edgeMap[coord].rg;

    float blendV = 0.0;
    float blendH = 0.0;

    if (edge.r > 0.0) {
        int left  = lineSearch(edgeMap, int2(coord), int2(-1,  0), data.blendMaxSteps, 0);
        int right = lineSearch(edgeMap, int2(coord), int2( 1,  0), data.blendMaxSteps, 0);
        blendV = areaFromLength(left, right);
    }

    if (edge.g > 0.0) {
        int up   = lineSearch(edgeMap, int2(coord), int2( 0, -1), data.blendMaxSteps, 1);
        int down = lineSearch(edgeMap, int2(coord), int2( 0,  1), data.blendMaxSteps, 1);
        blendH = areaFromLength(up, down);
    }

    weightBuffer[coord] = float4(blendH, blendV, 0.0, 0.0);
}