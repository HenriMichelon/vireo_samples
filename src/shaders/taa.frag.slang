// https://alextardif.com/TAA.html
#include "postprocess.inc.slang"

ConstantBuffer<Params> params  : register(b0);
Texture2D inputImage           : register(t1);
Texture2D history              : register(t2);
Texture2D velocity             : register(t3);
SamplerState sampler           : register(SAMPLER_LINEAR_EDGE, space1);

float3 RGBToYCoCg(float3 rgb) {
    float Y  = dot(rgb, float3(0.25, 0.5, 0.25));
    float Co = dot(rgb, float3(0.5, 0.0, -0.5));
    float Cg = dot(rgb, float3(-0.25, 0.5, -0.25));
    return float3(Y, Co, Cg);
}

float3 YCoCgToRGB(float3 ycocg) {
    float Y  = ycocg.x;
    float Co = ycocg.y;
    float Cg = ycocg.z;
    return float3(Y + Co - Cg, Y + Cg, Y - Co - Cg);
}

// Catmull-Rom filtering for sharper history sampling
float4 SampleTextureCatmullRom(Texture2D tex, SamplerState samp, float2 uv, float2 texSize) {
    float2 position = uv * texSize;
    float2 center = floor(position - 0.5) + 0.5;
    float2 f = position - center;
    float2 f2 = f * f;
    float2 f3 = f2 * f;
    
    float2 w0 = -0.5 * f3 + f2 - 0.5 * f;
    float2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;
    float2 w2 = -1.5 * f3 + 2.0 * f2 + 0.5 * f;
    float2 w3 = 0.5 * f3 - 0.5 * f2;
    
    float2 w12 = w1 + w2;
    float2 tc12 = (center + w2 / w12) / texSize;
    float2 tc0 = (center - 1.0) / texSize;
    float2 tc3 = (center + 2.0) / texSize;
    
    float4 result =
        tex.Sample(samp, float2(tc0.x, tc0.y)) * (w0.x * w0.y) +
        tex.Sample(samp, float2(tc12.x, tc0.y)) * (w12.x * w0.y) +
        tex.Sample(samp, float2(tc3.x, tc0.y)) * (w3.x * w0.y) +
        tex.Sample(samp, float2(tc0.x, tc12.y)) * (w0.x * w12.y) +
        tex.Sample(samp, float2(tc12.x, tc12.y)) * (w12.x * w12.y) +
        tex.Sample(samp, float2(tc3.x, tc12.y)) * (w3.x * w12.y) +
        tex.Sample(samp, float2(tc0.x, tc3.y)) * (w0.x * w3.y) +
        tex.Sample(samp, float2(tc12.x, tc3.y)) * (w12.x * w3.y) +
        tex.Sample(samp, float2(tc3.x, tc3.y)) * (w3.x * w3.y);
    
    return result;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float2 uv = input.uv;
    float2 vel = velocity.Sample(sampler, uv).xy;
    float2 historyUV = uv - vel;
    
    float4 current = inputImage.Sample(sampler, uv);
    
    if (any(historyUV < 0.0) || any(historyUV > 1.0)) {
        return current;
    }
    
    float4 prev = SampleTextureCatmullRom(history, sampler, historyUV, params.imageSize);
    float3 currentYCoCg = RGBToYCoCg(current.rgb);
    float3 prevYCoCg = RGBToYCoCg(prev.rgb);
    
    float3 m1 = 0.0, m2 = 0.0;
    float3 neighborMin = float3(1e10, 1e10, 1e10);
    float3 neighborMax = float3(-1e10, -1e10, -1e10);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float2 offset = float2(x, y) / params.imageSize;
            float3 neighborYCoCg = RGBToYCoCg(inputImage.Sample(sampler, uv + offset).rgb);
            m1 += neighborYCoCg;
            m2 += neighborYCoCg * neighborYCoCg;
            neighborMin = min(neighborMin, neighborYCoCg);
            neighborMax = max(neighborMax, neighborYCoCg);
        }
    }
    
    // Variance clipping with rounded corners
    float3 mean = m1 / 9.0;
    float3 stddev = sqrt(max(0.0, (m2 / 9.0) - (mean * mean)));
    //float3 minColor = mean - 1.25 * stddev;
    //float3 maxColor = mean + 1.25 * stddev;
    float3 minColor = mean - 1.0 * stddev;
    float3 maxColor = mean + 1.0 * stddev;

    // Hybrid min-max + variance clipping
    minColor = max(minColor, neighborMin);
    maxColor = min(maxColor, neighborMax);
    
    float3 clampedPrevYCoCg = clamp(prevYCoCg, minColor, maxColor);
    
    // Adaptive blend factor based on velocity and clamping amount
    float velocityWeight = saturate(length(vel) * 50.0);
    //float velocityWeight = saturate(length(vel) * 30.0);
    float clampAmount = length(clampedPrevYCoCg - prevYCoCg);
    float clampWeight = saturate(clampAmount * 5.0);
    //float clampWeight = saturate(clampAmount * 3.0);

    float blendFactor = lerp(0.92, 0.7, max(velocityWeight, clampWeight));
    //float blendFactor = lerp(0.95, 0.75, max(velocityWeight, clampWeight));

    float3 finalPrevRGB = YCoCgToRGB(clampedPrevYCoCg);
    float3 result = lerp(current.rgb, finalPrevRGB, blendFactor);
    
    return float4(result, current.a);
}