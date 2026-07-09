cbuffer TextVertexUniforms : register(b0, space1) {
    float4 rect_pixels;   // x, y (top-left), w, h
    float2 screen_size;
    float2 padding;
};

struct VSOutput {
    float2 uv : TEXCOORD0;
    float4 position : SV_Position;
};

VSOutput main(uint vertex_id : SV_VertexID) {
    float2 uv = float2(vertex_id & 1, vertex_id >> 1);
    float2 pixel_pos = rect_pixels.xy + uv * rect_pixels.zw;

    VSOutput output;
    output.position = float4(
        (pixel_pos.x / screen_size.x) * 2.0f - 1.0f,
        1.0f - (pixel_pos.y / screen_size.y) * 2.0f,
        0.0f,
        1.0f
    );
    output.uv = uv;
    return output;
}
