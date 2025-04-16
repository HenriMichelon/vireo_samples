struct Params {
    uint2 imageSize;
};

ConstantBuffer<Params> params  : register(b0);
RWTexture2D outputImage        : register(u1);

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint2 coord = dispatchThreadID.xy;
    uint width, height;
    width = params.imageSize.x;
    height = params.imageSize.y;
    if (coord.x >= width || coord.y >= height) { return; }

    float2 uv = float2(coord) / float2(width, height);
    outputImage[coord] = float4(uv.x, uv.y, 0.5, 1.0);
}