/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "postprocess.inc.slang"

ConstantBuffer<Params> params  : register(b0);
Texture2D inputImage           : register(t1);
SamplerState sampler           : register(s0, space1);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float3 c = inputImage.Sample(sampler, input.uv).rgb;
    float3 n = inputImage.Sample(sampler, input.uv + float2(0, -1) / params.imageSize).rgb;
    float3 w = inputImage.Sample(sampler, input.uv + float2(-1, 0) / params.imageSize).rgb;

    float luma = dot(c, float3(0.299, 0.587, 0.114));
    float lumaN = dot(n, float3(0.299, 0.587, 0.114));
    float lumaW = dot(w, float3(0.299, 0.587, 0.114));

    float edgeH = abs(luma - lumaN);
    float edgeV = abs(luma - lumaW);

    return float4(edgeH, edgeV, 0, 0);
}
