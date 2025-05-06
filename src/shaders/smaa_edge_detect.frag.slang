/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "postprocess.inc.slang"

ConstantBuffer<Params> params  : register(b0);
Texture2D inputImage           : register(t1);
SamplerState sampler           : register(SAMPLER_LINEAR_EDGE, space1);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float3 lumaWeight = float3(0.299, 0.587, 0.114);
    float lumaC  = dot(inputImage.Sample(sampler, input.uv).rgb, lumaWeight);
    float lumaN  = dot(inputImage.Sample(sampler, input.uv + float2(0, -1) / params.imageSize).rgb, lumaWeight);
    float lumaS  = dot(inputImage.Sample(sampler, input.uv + float2(0, 1) / params.imageSize).rgb, lumaWeight);
    float lumaW  = dot(inputImage.Sample(sampler, input.uv + float2(-1, 0) / params.imageSize).rgb, lumaWeight);
    float lumaE  = dot(inputImage.Sample(sampler, input.uv + float2(1, 0) / params.imageSize).rgb, lumaWeight);

    float edgeH = abs(lumaW - lumaC) + abs(lumaC - lumaE);
    float edgeV = abs(lumaN - lumaC) + abs(lumaC - lumaS);

    return float4(edgeH, edgeV, 0, 0);
}
