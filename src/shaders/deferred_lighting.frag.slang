/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "lighting.inc.slang"

struct VertexOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

ConstantBuffer<Global> global : register(b0);
ConstantBuffer<Light>  light  : register(b1);
Texture2D    positionBuffer   : register(t2);
Texture2D    normalBuffer     : register(t3);
Texture2D    albedoBuffer     : register(t4);
Texture2D    materialBuffer   : register(t5);
SamplerState sampler          : register(SAMPLER_NEAREST_BORDER, space1);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float3 worldPos = positionBuffer.Sample(sampler, input.uv).rgb;
    float3 normal = normalBuffer.Sample(sampler, input.uv).rgb;
    float2 material = materialBuffer.Sample(sampler, input.uv).rg;
    float3 albedo = albedoBuffer.Sample(sampler, input.uv).rgb;
    float3 lit = calcLighting(global, light, worldPos, normal, material.r, material.g);
    return float4(lit * albedo, 1.0);
}
