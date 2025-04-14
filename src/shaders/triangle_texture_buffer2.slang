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
    uint instanceID : SV_InstanceID;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
    float3 color    : COLOR;
};

struct UBO1 {
    float4 offset;
    float  scale;
};

struct UBO2 {
    float3 color;
};

ConstantBuffer<UBO1> ubo1   : register(b0);
ConstantBuffer<UBO2> ubo2   : register(b1);
Texture2D texture[1]        : register(t2);
SamplerState samplers[1]    : register(s0, space1);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    output.position = input.position;
    output.position.y -= 0.5;
    output.position.x += input.instanceID == 0 ? -0.5 : 0.5;
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    return float4(input.uv.x, input.uv.y, 0.0, 1.0)+ float4(ubo2.color + input.color, 1.0);
}
