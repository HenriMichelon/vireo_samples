/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct VertexInput {
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 color    : COLOR;
};

struct Global {
    float4x4 projection;
    float4x4 view;
}

struct Model {
    float4x4 transform;
}

ConstantBuffer<Global> global : register(b0);
ConstantBuffer<Model>  model  : register(b1);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    output.position = mul(global.projection, mul(global.view, mul(model.transform, float4(input.position, 1.0))));
    output.color = input.color;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    return float4(input.color, 1.0);
}
