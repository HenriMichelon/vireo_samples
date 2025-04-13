/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct VertexInput {
    float4 position : POSITION;
    float3 color    : COLOR;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 color    : COLOR;
};

VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    output.position = input.position ;
    output.color = input.color;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_TARGET {
    return float4(input.color, 1.0);
}
