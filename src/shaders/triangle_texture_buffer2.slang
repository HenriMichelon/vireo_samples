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

struct PushConstants {
    float3 color;
};

[[push_constant]]
PushConstants pushConstants : register(b0, space2);

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
    return float4(input.uv.x, input.uv.y, 0.0, 1.0)+ float4(pushConstants.color + input.color, 1.0);
}
