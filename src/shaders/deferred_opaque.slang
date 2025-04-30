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

ConstantBuffer<Global> global : register(b0);
ConstantBuffer<Model>  model  : register(b1);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    output.position = mul(global.projection, mul(global.view, mul(model.transform, float4(input.position, 1.0))));
    output.uv = input.uv;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    return float4(input.uv.x, input.uv.y, 0.0, 1.0);
}
