/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct VertexInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : UV;
    float3 tangent  : TANGENT;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float2 uv       : TEXCOORD2;
    float3 tangent  : TEXCOORD3;
    float3 bitangent: TEXCOORD4;
};

struct FragmentOutput {
    float4   position : SV_TARGET0; // xyz = world-space position
    float4   normal   : SV_TARGET1; // xyz = world-space normal
    float4   albedo   : SV_TARGET2; // rgb = albedo, a unused
    float4   rma      : SV_TARGET3; // r=roughness, g=metallic, b=AO, a unused
};

struct Global {
    float3   cameraPosition;
    float4x4 projection;
    float4x4 view;
    float4x4 viewInverse;
    float4   ambientLight;
}

struct Model {
    float4x4 transform;
}

struct Material {
    float shininess;
    int   diffuseTextureIndex;
    int   normalTextureIndex;
    int   aoTextureIndex;
}

ConstantBuffer<Global>   global   : register(b0);
ConstantBuffer<Model>    model    : register(b1);
ConstantBuffer<Material> material : register(b2);
Texture2D    textures[3]          : register(t3);
SamplerState sampler              : register(s0, space1);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;

    float4 localPos = float4(input.position, 1.0);
    float4 worldPos = mul(model.transform, localPos);
    float4 viewPos = mul(global.view, worldPos);
    output.position = mul(global.projection, viewPos);

    float3 normalW = mul((float3x3)model.transform, input.normal);
    output.normal = normalize(normalW);

    float3 tangentW = mul((float3x3)model.transform, input.tangent);
    float3 bitangentW = normalize(cross(normalW, tangentW));
    output.tangent = normalize(tangentW);
    output.bitangent = bitangentW;

    output.uv = input.uv;
    return output;
}

FragmentOutput fragmentMain(VertexOutput input) {
    FragmentOutput output;
    float3x3 TBN = (float3x3(input.tangent, input.bitangent, input.normal));
    float4 color = textures[material.diffuseTextureIndex].Sample(sampler, input.uv);
    float3 normal = textures[material.normalTextureIndex].Sample(sampler, input.uv).rgb;
    float ao = textures[material.aoTextureIndex].Sample(sampler, input.uv).r;
    float3 N = normalize(normal * 2.0 - 1.0);
    N = normalize(mul(TBN, N));

    output.position = float4(input.worldPos.xyz, 1.0);
    output.normal = float4(N, 0.0);
    output.albedo = float4(color.rgb, 1.0);
    output.rma = float4(0.0, 0.0, ao, 0.0);
    return output;
}
