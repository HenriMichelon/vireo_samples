/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "postprocess.inc.slang"

ConstantBuffer<Params> params  : register(b0);
Texture2D inputImage           : register(t1);
Texture2D blendBuffer          : register(t2);
SamplerState sampler           : register(s0, space1);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float4 color = inputImage.Sample(sampler, input.uv);
    float2 blend = blendBuffer.Sample(sampler, input.uv).rg;
    float4 n = inputImage.Sample(sampler, input.uv + float2(0, -1) / params.imageSize);
    float4 e = inputImage.Sample(sampler, input.uv + float2(1, 0) / params.imageSize);

    float4 blended = lerp(color, n, blend.r);
    blended = lerp(blended, e, blend.g);
    return blended;
}
