/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct VertexInput {
    float3 position : POSITION;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 uv : TEXCOORD;
};

struct Global {
    float4x4 projection;
    float4x4 view;
}

ConstantBuffer<Global> global : register(b0);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    float4 pos = mul(global.projection, mul(global.view, float4(input.position, 1.0)));
    output.position = float4(pos.xy, pos.w, pos.w);
    output.uv = input.position;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    return float4(input.uv.x, input.uv.y, input.uv.z, 1.0);
}
