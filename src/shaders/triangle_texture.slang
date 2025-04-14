/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct VertexInput {
    float4 position : POSITION;
    float2 uv       : TEXCOORD;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

Texture2D texture[1]        : register(t0);
SamplerState samplers[1]    : register(s0, space1);

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    output.position = input.position;
    output.uv = input.uv;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    return texture[0].Sample(samplers[0], input.uv);
}
