struct Params {
    uint2 imageSize;
};

ConstantBuffer<Params> params  : register(b0);
RWTexture2D outputImage        : register(t1);

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint2 coord = dispatchThreadID.xy;
    if (coord.x >= params.imageSize.x || coord.y >= params.imageSize.y) {
        return;
    }

    float2 uv = float2(coord) / float2(params.imageSize);
    float4 color = float4(uv.x, uv.y, 0.5, 1.0); // Dégradé simple
    outputImage[coord] = color;
}