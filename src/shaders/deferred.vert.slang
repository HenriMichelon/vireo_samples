/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
#include "global.inc.slang"
#include "scene_input.inc.slang"

struct Models {
    Model models[2];
}

struct PushConstants {
    uint modelIndex;
    uint materialIndex;
};

[[push_constant]]
PushConstants pushConstants : register(b0, space2);

ConstantBuffer<Global> global : register(b0);
ConstantBuffer<Models> models : register(b1);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    float4x4 model = models.models[pushConstants.modelIndex].transform;

    float4 localPos = float4(input.position, 1.0);
    float4 worldPos = mul(model, localPos);
    output.worldPos = worldPos.xyz;

    float4 viewPos = mul(global.view, worldPos);
    output.position = mul(global.projection, viewPos);

    float3 normalW = mul((float3x3)model, input.normal);
    output.normal = normalize(normalW);

    float3 tangentW = mul((float3x3)model, input.tangent);
    float3 bitangentW = normalize(cross(normalW, tangentW));
    output.tangent = normalize(tangentW);
    output.bitangent = bitangentW;

    output.uv = input.uv;
    return output;
}
