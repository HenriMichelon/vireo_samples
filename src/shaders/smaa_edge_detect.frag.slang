/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "smaa_common.inc.slang"

// SMAA Edge Detection (luma-based)
// Ref: Jimenez et al., "SMAA: Enhanced Morphological Antialiasing", EGSR 2012
// https://www.iryoku.com/smaa/
static const float SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR = 2.0;

ConstantBuffer<Params> params : register(b0);
ConstantBuffer<Data>   data   : register(b1);
Texture2D inputImage          : register(t2);

float luma(float3 c) {
    return dot(c, float3(0.299, 0.587, 0.114));
}

float2 fragmentMain(VertexOutput input) : SV_TARGET {
    int2 coord = int2(input.position.xy);

    float lumaC  = luma(inputImage.Load(int3(coord,                  0)).rgb);
    float lumaN  = luma(inputImage.Load(int3(coord + int2( 0, -1), 0)).rgb);
    float lumaS  = luma(inputImage.Load(int3(coord + int2( 0,  1), 0)).rgb);
    float lumaW  = luma(inputImage.Load(int3(coord + int2(-1,  0), 0)).rgb);
    float lumaE  = luma(inputImage.Load(int3(coord + int2( 1,  0), 0)).rgb);

    float deltaLeft  = abs(lumaW - lumaC);
    float deltaRight = abs(lumaC - lumaE);
    float deltaUp    = abs(lumaN - lumaC);
    float deltaDown  = abs(lumaC - lumaS);

    float deltaH = deltaLeft  + deltaRight;
    float deltaV = deltaUp    + deltaDown;

    float lumaN2 = luma(inputImage.Load(int3(coord + int2( 0, -2), 0)).rgb);
    float lumaS2 = luma(inputImage.Load(int3(coord + int2( 0,  2), 0)).rgb);
    float lumaW2 = luma(inputImage.Load(int3(coord + int2(-2,  0), 0)).rgb);
    float lumaE2 = luma(inputImage.Load(int3(coord + int2( 2,  0), 0)).rgb);

    float outerH = abs(lumaW2 - lumaW) + abs(lumaE - lumaE2);
    float outerV = abs(lumaN2 - lumaN) + abs(lumaS - lumaS2);

    float localContrast    = max(outerH, outerV);
    float adaptedThreshold = max(data.edgeThreshold,
                                 SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * localContrast);

    float edgeH = (deltaH > adaptedThreshold) ? deltaH : 0.0;
    float edgeV = (deltaV > adaptedThreshold) ? deltaV : 0.0;

    return float2(edgeH, edgeV);
}