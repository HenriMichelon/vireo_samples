/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "postprocess.inc.slang"

ConstantBuffer<Params> params : register(b0);
Texture2D inputImage          : register(t1);
SamplerState sampler          : register(s0, space1);

float3 linearToSrgb(float3 c) {
    return select(
        c <= 0.0031308,
        c * 12.92,
        1.055 * pow(c, 1.0 / 2.4) - 0.055
    );
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float4 color = inputImage.Sample(sampler, input.uv);
    return float4(linearToSrgb(color.rgb), color.a);
}
