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

Texture2D    colorBuffer : register(t0);
Texture2D    alphaBuffer : register(t1);
SamplerState sampler     : register(s0, space1);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float4 accum = colorBuffer.Sample(sampler, input.uv);
    float a = 1.0 - accum.a;
    accum.a = alphaBuffer.Sample(sampler, input.uv).r;
    return float4(a * accum.rgb / clamp(accum.a, 0.001, 50000.0), a);
}
