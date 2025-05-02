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

ConstantBuffer<Params> params : register(b0);
Texture2D inputImage          : register(t1);
SamplerState sampler          : register(s0, space1);

float2 hash22(float2 p) {
    float n = sin(dot(p, float2(41, 289)));
    return frac(float2(sin(n * 12.9898), cos(n * 78.233)));
}

float hash1(float2 p) {
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float voronoi(float2 uv, float time) {
    float2 g = floor(uv);
    float2 f = frac(uv);

    float minDist = 1.0;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float2 offset = float2(x, y);
            float2 cell = g + offset;

            float phase = hash1(cell) * 6.2831; // [0, 2PI]
            float scale = 0.5 + 0.5 * sin(time * 2.0 + phase); // [0, 1]

            float2 jitter = hash22(cell) - 0.5;
            float2 pos = offset + jitter * scale;

            float2 diff = pos - f;
            float dist = dot(diff, diff);
            minDist = min(minDist, dist);
        }
    }

    return sqrt(minDist);
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float tiling = 10.0;
    float mask = voronoi(input.uv * tiling, params.time / 1000.0);
    float4 color = inputImage.Sample(sampler, input.uv);
    float strength = saturate(mask);
    color = lerp(color, float4(1.0, 1.0, 1.0, 1), strength * 0.4);
//     color = clamp(color * (0.8 + strength), 0.0, 0.7);
    return float4(color.rgb, 1.0);
}
