sampler2D iChannel0 : register(s0);
float2 iResolution : register(c0);
float2 iTargetResolution : register(c1);

float4 LoadScreenPS(float2 PixelPos : VPOS) : COLOR0 {
    float2 grid = iResolution / iTargetResolution;
    float2 uv = (PixelPos - (PixelPos % grid) + (grid / 2)) / iResolution;
    return tex2D(iChannel0, uv);
}
