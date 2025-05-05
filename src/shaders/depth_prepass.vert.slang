/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "global.inc.slang"

struct VertexInput {
    float3 position : POSITION;
};

ConstantBuffer<Global> global : register(b0, space0);
ConstantBuffer<Model>  model  : register(b0, space1);

float4 vertexMain(VertexInput input) : SV_POSITION {
    float4 localPos = float4(input.position, 1.0);
    float4 worldPos = mul(model.transform, localPos);
    float4 viewPos = mul(global.view, worldPos);
    return mul(global.projection, viewPos);
}
