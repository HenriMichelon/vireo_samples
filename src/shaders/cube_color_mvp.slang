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
    float2 uv       : UV;
};

struct Global {
    float4x4 projection;
    float4x4 view;
}

struct Model {
    float4x4 transform;
}

struct Material {
    int diffuseTextureIndex;
}

ConstantBuffer<Global>   global   : register(b0);
ConstantBuffer<Model>    model    : register(b1);
ConstantBuffer<Material> material : register(b2);
Texture2D textures[1]             : register(t3);
SamplerState sampler              : register(s0, space1);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    output.position = mul(global.projection, mul(global.view, mul(model.transform, float4(input.position, 1.0))));
    output.uv = input.uv;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
     float4 color = textures[material.diffuseTextureIndex].Sample(sampler, input.uv);
    return color;
}
