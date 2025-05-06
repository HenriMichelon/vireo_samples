/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "global.inc.slang"

struct VertexOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D    accumBuffer  : register(t0);
Texture2D    revealBuffer : register(t1);
SamplerState sampler      : register(SAMPLER_NEAREST_BORDER, space1);

// https://learnopengl.com/Guest-Articles/2020/OIT/Weighted-Blended
bool isApproximatelyEqual(float a, float b) {
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * 0.00001f;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float revealage = revealBuffer.Sample(sampler, input.uv).r;
    if (isApproximatelyEqual(revealage, 1.0f)) {
        discard;
    }
    float4 accum = accumBuffer.Sample(sampler, input.uv);
    return float4(accum.rgb / max(accum.a, 0.00001f), revealage);
}
