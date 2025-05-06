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

ConstantBuffer<Params> params : register(b0);
RWTexture2D outputImage       : register(u1);

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint2 coord = dispatchThreadID.xy;
    if (coord.x >= params.imageSize.x || coord.y >=  params.imageSize.y) { return; }
    float2 uv = float2(coord) / params.imageSize;

    const float frequency = 20.0;
    const float amplitude = 0.03;
    const float bandHeight = 0.05;
    const float speedX = 0.2;
    const float speedY = 2.0;
    const float lightingStrength = 0.75;

    float waveAmplitude = amplitude * (0.5 + 0.5 * sin(params.time * speedY)) + amplitude;
    float wavePhase = (uv.x + params.time * speedX) * frequency;
    float wave = sin(wavePhase) * waveAmplitude;
    float slope = cos(wavePhase) * frequency * waveAmplitude;

    int waveStripeIndex = int((uv.y + wave) / bandHeight);
    bool isEvenStripe = (waveStripeIndex % 2) == 0;

    float3 color;
    if (isEvenStripe) {
        float2 uvUp = float2(coord.x, coord.y-1) / params.imageSize;
        float2 uvDown = float2(coord.x, coord.y+1) / params.imageSize;
        if (coord.y > 0 && ((int((uvUp.y + wave) / bandHeight) % 2 != 0) || (int((uvDown.y + wave) / bandHeight) % 2 != 0))) {
            color = float3(1.0);
        } else {
            color = float3(uv.x + 0.25, uv.y, .5);
        }
    } else {
        color = float3(0.0);
    }
//     float3 color = isEvenStripe ? float3(uv.x + 0.25, uv.y, .5) : float3(0.0f);
    float lighting = saturate(1.0 - slope * lightingStrength);
    outputImage[coord] = float4(color * lighting, 1.0);
}