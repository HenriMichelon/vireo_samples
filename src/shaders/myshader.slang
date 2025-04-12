/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct VertexInput {
    float4 position : POSITION;
    float2 uv       : TEXCOORD;
    float3 color    : COLOR;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
    float3 color    : COLOR;
};

struct UBO1 {
    float4 offset;
    float scale;
};

struct UBO2 {
    float3 color;
};

ConstantBuffer<UBO1> ubo1   : register(b0);
ConstantBuffer<UBO2> ubo2   : register(b1);
Texture2D texture[2]        : register(t2);
SamplerState samplers[2]    : register(s0, space1);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    output.position = input.position + ubo1.offset;
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    return texture[1].Sample(samplers[0], input.uv) + float4(ubo2.color + input.color, 1.0);
}
