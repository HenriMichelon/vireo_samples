// [SIG2014] - Total Noob
// https://www.shadertoy.com/view/XdlSDs

struct Params {
    uint2 imageSize;
    float time;
};

ConstantBuffer<Params> params  : register(b0);
RWTexture2D outputImage        : register(u1);

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
    float2 p = (dispatchThreadID.xy * 2.0f - params.imageSize.xy) / (float)params.imageSize.y;
    float tau = float.getPi() * 2.0;
    float a = atan2(p.x,p.y);
    float r = length(p)*0.75;
    float2 uv = float2(a/tau,r);

    //get the color
    float xCol = (uv.x - params.time/3) * 3.0;
    xCol = fmod(abs(xCol), 3.0f);
    float3 horColour = float3(0.25, 0.25, 0.25);

    if (xCol < 1.0) {
        horColour.r += 1.0 - xCol;
        horColour.g += xCol;
    }
    else if (xCol < 2.0) {
        xCol -= 1.0;
        horColour.g += 1.0 - xCol;
        horColour.b += xCol;
    }
    else {
        xCol -= 2.0;
        horColour.b += 1.0 - xCol;
        horColour.r += xCol;
    }

    // draw color beam
    uv = (2.0 * uv) - 1.0;
    float beamWidth = (0.7 +
        0.5 * cos(uv.x * 10.0 * tau * 0.15 * clamp(floor(5.0 + 10.0 * cos(params.time)), 0.0, 10.0)))
        * abs(1.0 / (30.0 * uv.y));
    float3 horBeam = float3(beamWidth);

    outputImage[dispatchThreadID.xy] = float4(((horBeam) * horColour), 1.0);
}