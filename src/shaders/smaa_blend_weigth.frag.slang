/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "postprocess.inc.slang"

ConstantBuffer<Params> params  : register(b0);
Texture2D inputImage           : register(t1);
Texture2D edgeBuffer           : register(t2);
SamplerState sampler           : register(s0, space1);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float2 edge = edgeBuffer.Sample(sampler, input.uv).rg;
    float weight = saturate(max(edge.r, edge.g));
    return float4(weight, weight, weight, 0);
}
