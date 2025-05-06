/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "global.inc.slang"
#include "scene_input.inc.slang"

struct FragmentOutput {
    float4   position : SV_TARGET0; // xyz = world-space position
    float4   normal   : SV_TARGET1; // xyz = world-space normal
    float4   albedo   : SV_TARGET2; // rgb = albedo, a unused
    float2   material : SV_TARGET3; // r=shininess, g=AO
};

struct Materials {
    Material materials[2];
}

struct PushConstants {
    uint modelIndex;
    uint materialIndex;
};

[[push_constant]]
PushConstants pushConstants : register(b0, space2);

ConstantBuffer<Materials> materials   : register(b2);
Texture2D                 textures[5] : register(t3);
SamplerState              sampler     : register(s0, space1);

FragmentOutput fragmentMain(VertexOutput input) {
    FragmentOutput output;
    Material material = materials.materials[pushConstants.materialIndex];

    float3x3 TBN = (float3x3(input.tangent, input.bitangent, input.normal));
    float4 color = textures[material.diffuseTextureIndex].Sample(sampler, input.uv);
    float3 normal = textures[material.normalTextureIndex].Sample(sampler, input.uv).rgb;
    float ao = material.aoTextureIndex != -1 ? textures[material.aoTextureIndex].Sample(sampler, input.uv).r : 1.0;
    float3 N = normalize(normal * 2.0 - 1.0);
    N = normalize(mul(TBN, N));

    output.position = float4(input.worldPos, 0.0);
    output.normal = float4(N, 0.0);
    output.albedo = float4(color.rgb, 1.0);
    output.material = float2(material.shininess, ao);
    return output;
}
