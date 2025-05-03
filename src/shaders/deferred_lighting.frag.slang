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

struct Light {
    float3 direction;
    float4 color;
}

ConstantBuffer<Global> global : register(b0);
ConstantBuffer<Model>  model  : register(b1);
ConstantBuffer<Light>  light  : register(b2);
Texture2D    positionBuffer   : register(t3);
Texture2D    normalBuffer     : register(t4);
Texture2D    albedoBuffer     : register(t5);
Texture2D    materialBuffer   : register(t6);
SamplerState sampler          : register(s0, space1);

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float3 worldPos = positionBuffer.Sample(sampler, input.uv).rgb;
    float3 normal = normalBuffer.Sample(sampler, input.uv).rgb;
    float3 albedo = albedoBuffer.Sample(sampler, input.uv).rgb;
    float2 material = materialBuffer.Sample(sampler, input.uv).rg;
    float shininess = material.r;
    float ao = material.g;

    float3 L = normalize(-light.direction);
    float3 V = normalize(global.cameraPosition - worldPos);
    float3 R = reflect(-L, normal);

    float diff = max(dot(normal, L), 0.0);
    float spec = pow(max(dot(V, R), 0.0), shininess);

    float3 diffuse = diff * light.color.rgb * light.color.w;
    float3 specular = spec * light.color.rgb * light.color.w;
    float3 ambient = global.ambientLight.rgb * global.ambientLight.w * ao;

    return float4((diffuse + specular + ambient) * albedo, 1.0);
}
