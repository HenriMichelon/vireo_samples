/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "postprocess.inc.slang"

ConstantBuffer<Params> params : register(b0);
Texture2D inputImage          : register(t1);
SamplerState sampler          : register(SAMPLER_NEAREST_BORDER, space1);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float4 color = inputImage.Sample(sampler, input.uv);
    const float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
    float3 result = float3(gray) * float3(1.0, 1.0, 1.0);
    return float4(clamp(result, 0.0, 1.0), 1.0);
}
