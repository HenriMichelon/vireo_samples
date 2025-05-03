/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct VertexInput {
    float3 position : POSITION;
};

struct Global {
    float3   cameraPosition;
    float4x4 projection;
    float4x4 view;
}

struct Model {
    float4x4 transform;
}

ConstantBuffer<Global> global : register(b0);
ConstantBuffer<Model>  model  : register(b1);

float4 vertexMain(VertexInput input) : SV_POSITION {
    float4 localPos = float4(input.position, 1.0);
    float4 worldPos = mul(model.transform, localPos);
    float4 viewPos = mul(global.view, worldPos);
    return mul(global.projection, viewPos);
}
