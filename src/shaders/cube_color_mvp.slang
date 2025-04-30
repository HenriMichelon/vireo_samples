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
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float2 uv       : TEXCOORD2;
};

struct Global {
    float4x4 projection;
    float4x4 view;
    float4x4 viewInverse;
    float4   ambientLight;
}

struct Model {
    float4x4 transform;
}

struct Light {
    float3 position;
    float4 color;
}

struct Material {
    int diffuseTextureIndex;
    int normalTextureIndex;
    int armTextureIndex;
}

ConstantBuffer<Global>   global   : register(b0);
ConstantBuffer<Model>    model    : register(b1);
ConstantBuffer<Material> material : register(b2);
ConstantBuffer<Light> light       : register(b3);
Texture2D textures[3]             : register(t4);
SamplerState sampler              : register(s0, space1);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;

    float4 localPos = float4(input.position, 1.0);
    float4 worldPos = mul(model.transform, localPos);
    output.worldPos = worldPos.xyz;

    float4 viewPos = mul(global.view, worldPos);
    output.position = mul(global.projection, viewPos);

    float3x3 normalMatrix = (float3x3)model.transform;
    output.normal = normalize(mul(normalMatrix, input.normal));

    output.uv = input.uv;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float3 cameraPos = global.viewInverse[3].xyz;

    float4 color = textures[material.diffuseTextureIndex].Sample(sampler, input.uv);
    float3 albedo = color.rgb;
//     float3 normal = textures[material.normalTextureIndex].Sample(sampler, input.uv).rgb;
     float roughness = textures[material.armTextureIndex].Sample(sampler, input.uv).g;
    float metallic = textures[material.armTextureIndex].Sample(sampler, input.uv).b;

    float3 N = normalize(input.normal);
    float3 L = normalize(light.position - input.worldPos);
    float3 V = normalize(cameraPos - input.worldPos);
    float3 R = reflect(-L, N);

    float k_d = 1.0 - metallic;
    float k_s = lerp(0.04, 1.0, metallic);
    float shininess = lerp(4.0, 128.0, 1.0 - roughness);
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(V, R), 0.0), shininess);

    float3 diffuse = k_d * albedo * diff * light.color.rgb;
    float3 specular = k_s * lerp(float3(1.0), albedo, metallic) * spec * light.color.rgb;
    float3 ambient = albedo * global.ambientLight.rgb * global.ambientLight.w;

    return float4(diffuse + specular + ambient, color.a);
}
