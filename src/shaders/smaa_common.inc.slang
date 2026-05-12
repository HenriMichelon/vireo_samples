/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

static const int TILE_SIZE = 16;
static const int INPUT_BUFFER  = 0;
static const int EDGE_BUFFER   = 1;
static const int WEIGHT_BUFFER = 2;

struct Params {
    uint2 imageSize;
    float time;
};

struct Data {
    float edgeThreshold;
    int   blendMaxSteps;
};

[[vk::binding(0, 0)]] ConstantBuffer<Params> params    : register(b0, space0);
[[vk::binding(1, 0)]] ConstantBuffer<Data>   data      : register(b1, space0);
[[vk::binding(2, 0)]] RWTexture2D<float4> outputImage  : register(u2, space0);
[[vk::binding(3, 0)]] Texture2D textures[]             : register(t3, space0);

[[vk::binding(0, 2)]] RWTexture2D<float4> edgeBuffer   : register(u0, space2);
[[vk::binding(1, 2)]] RWTexture2D<float4> weightBuffer : register(u1, space2);