/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

struct VertexOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct Params {
    uint2 imageSize;
    float time;
};

ConstantBuffer<Params> params  : register(b0);
Texture2D inputImage           : register(t1);
SamplerState sampler           : register(s0, space1);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
   float dx = 15.*(1./512.);
   float dy = 10.*(1./512.);
   float2 coord = float2(dx*floor(input.uv.x/dx),
                     dy*floor(input.uv.y/dy));
   return inputImage.Sample(sampler, coord);
}
