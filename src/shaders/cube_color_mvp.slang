/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "global.inc.slang"
#include "scene_input.inc.slang"

ConstantBuffer<Global>    global      : register(b0, space0);
ConstantBuffer<Light>     light       : register(b1, space0);
Texture2D                 textures[6] : register(t2, space0);
SamplerState              sampler     : register(s0, space1);
ConstantBuffer<Model>     model       : register(b0, space2);
ConstantBuffer<Material>  material    : register(b0, space3);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;

    float4 localPos = float4(input.position, 1.0);
    float4 worldPos = mul(model.transform, localPos);
    output.worldPos = worldPos.xyz;

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

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float3x3 TBN = (float3x3(input.tangent, input.bitangent, input.normal));

    float4 color = textures[material.diffuseTextureIndex].Sample(sampler, input.uv);
    float3 normal = textures[material.normalTextureIndex].Sample(sampler, input.uv).rgb;
    float ao = textures[material.aoTextureIndex].Sample(sampler, input.uv).r;

    float3 albedo = color.rgb;
    float3 N = normalize(normal * 2.0 - 1.0);
    N = normalize(mul(TBN, N));
//     float3 N = normalize(input.normal);

    float3 L = normalize(-light.direction);
    float3 V = normalize(global.cameraPosition - input.worldPos);
    float3 R = reflect(-L, N);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(V, R), 0.0), material.shininess);

    float3 diffuse = diff * light.color.rgb * light.color.w;
    float3 specular = spec * light.color.rgb * light.color.w;
    float3 ambient = global.ambientLight.rgb * global.ambientLight.w * ao;

    return float4((diffuse + specular + ambient) * albedo, color.a);
//     return float4(ao, 0.0, 0.0, color.a);
}
