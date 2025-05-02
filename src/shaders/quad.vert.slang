/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct VertexOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    output.position.y = - output.position.y;
    return output;
}
