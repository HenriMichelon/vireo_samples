/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "lighting.inc.slang"
#include "scene_input.inc.slang"

struct FragmentOutput {
    float4 color : SV_TARGET0;
    float  alpha : SV_TARGET1;
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

ConstantBuffer<Global>    global      : register(b0);
ConstantBuffer<Light>     light       : register(b2);
ConstantBuffer<Materials> materials   : register(b3);
Texture2D                 textures[5] : register(t4);
SamplerState              sampler     : register(s0, space1);


 float weight(float z, float a) {
    return clamp(pow(min(1.0, a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - z * 0.9, 3.0), 1e-2, 3e3);
}

FragmentOutput fragmentMain(VertexOutput input) {
    FragmentOutput output;
    Material material = materials.materials[pushConstants.materialIndex];
    float3x3 TBN = (float3x3(input.tangent, input.bitangent, input.normal));
    float4 color = textures[material.diffuseTextureIndex].Sample(sampler, input.uv);
    float3 normal = textures[material.normalTextureIndex].Sample(sampler, input.uv).rgb;
    float ao = material.aoTextureIndex != -1 ? textures[material.aoTextureIndex].Sample(sampler, input.uv).r : 1.0;
    float3 N = normalize(normal * 2.0 - 1.0);
    N = normalize(mul(TBN, N));
    float3 lit = calcLighting(global, light, input.worldPos, N, material.shininess, ao);

    float w = weight(input.position.z, color.a);
    output.color = float4(color.rgb * w, color.a);
    output.alpha = color.a * w;
    return output;
}
