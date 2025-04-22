/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct Params {
    uint2 imageSize;
    float time;
};

ConstantBuffer<Params> params  : register(b0);
Texture2D inputImage           : register(t1);
RWTexture2D outputImage        : register(u2);
SamplerState sampler           : register(s0, space1);

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint2 coord = dispatchThreadID.xy;
    if (coord.x >= params.imageSize.x || coord.y >=  params.imageSize.y) { return; }
    float2 uv = float2(coord) / params.imageSize;
    float4 color = inputImage.Sample(sampler, uv);
//     float luminance = dot(color.rgb, float3(0.299, 0.587, 0.114));
//     outputImage[coord] = float4(luminance.xxx, 1.0);
    outputImage[coord] = float4(uv.x, uv.y, 0.5, 1.0);
}