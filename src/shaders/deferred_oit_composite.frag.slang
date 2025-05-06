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

Texture2D    accumBuffer : register(t0);
Texture2D    revealBuffer : register(t1);
SamplerState sampler     : register(s0, space1);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float4 accum = accumBuffer.Sample(sampler, input.uv);
    float reveal = revealBuffer.Sample(sampler, input.uv).r;
    return float4(accum.rgb / max(accum.a, 1e-5), reveal);
}
