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

    float3 normalW = mul((float3x3)model.transform, input.normal);
    output.normal = normalize(normalW);

    float3 tangentW = mul((float3x3)model.transform, input.tangent);
    float3 bitangentW = normalize(cross(normalW, tangentW));
    output.tangent = normalize(tangentW);
    output.bitangent = bitangentW;

    output.uv = input.uv;
    return output;
}

// Helper: decode normal map (tangent space)
float3 decodeNormal(Texture2D tex, float2 uv)
{
    float3 n = textures[material.normalTextureIndex].Sample(sampler, uv).xyz * 2.0 - 1.0;
    return normalize(n);
}

// Fresnel-Schlick approximation
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX NDF
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (3.1416 * denom * denom);
}

// Geometry: Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}
/*
float4 fragmentMain(VertexOutput input) : SV_TARGET {
        float3 V = normalize(global.cameraPosition - input.worldPos);
        float3 N = decodeNormal(textures[material.normalTextureIndex], input.uv);
        float3 L = normalize(-light.direction);
        float3 H = normalize(V + L);

        float3 albedo = pow(textures[material.diffuseTextureIndex].Sample(sampler, input.uv).rgb, 2.2); // gamma to linear
        float roughness = saturate(textures[material.armTextureIndex].Sample(sampler, input.uv).g);
        float metalness = saturate(textures[material.armTextureIndex].Sample(sampler, input.uv).b);

        float3 F0 = lerp(float3(0.04), albedo, metalness);

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        float3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);

        float3 specular = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 1e-5);

        float3 kD = (1.0 - F) * (1.0 - metalness);
        float NdotL = max(dot(N, L), 0.0);

        float3 radiance = light.color.rgb;

        float3 Lo = (kD * albedo / 3.1416 + specular) * radiance * NdotL;

        return float4(albedo, 1.0);
}
*/
float4 fragmentMain(VertexOutput input) : SV_TARGET {
    float3 cameraPos = global.viewInverse[3].xyz;
    float3x3 TBN = (float3x3(input.tangent, input.bitangent, input.normal));

    float4 color = textures[material.diffuseTextureIndex].Sample(sampler, input.uv);
    float3 albedo = color.rgb;
    float3 normal = textures[material.normalTextureIndex].Sample(sampler, input.uv).rgb;
//     float roughness = textures[material.armTextureIndex].Sample(sampler, input.uv).g;
//     float metallic = textures[material.armTextureIndex].Sample(sampler, input.uv).b;

    float3 N = normalize(normal * 2.0 - 1.0);
    N = normalize(mul(TBN, N));
//     float3 N = normalize(input.normal);
    float3 L = normalize(-light.direction);
    float3 V = normalize(cameraPos - input.worldPos);
    float3 R = reflect(-L, N);

//     float k_d = 1.0 - metallic;
//     float k_s = lerp(0.04, 1.0, metallic);
//     float shininess = lerp(4.0, 128.0, 1.0 - roughness);
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(V, R), 0.0), 32.0); //shininess);

//     float3 diffuse = k_d * albedo * diff * light.color.rgb;
//     float3 specular = k_s * lerp(float3(1.0), albedo, metallic) * spec * light.color.rgb;
//     float3 ambient = albedo * global.ambientLight.rgb * global.ambientLight.w;
    float3 diffuse = diff * light.color.rgb;
    float3 specular = spec * light.color.rgb;
    float3 ambient = global.ambientLight.rgb * global.ambientLight.w;

    return float4((diffuse + specular + ambient) * albedo, color.a);
}
