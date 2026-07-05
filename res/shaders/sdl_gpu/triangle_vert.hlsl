struct VSOutput {
    float4 position : SV_Position;
};

VSOutput main(uint vertex_id : SV_VertexID) {
    float2 positions[3] = {
        float2( 0.0f,  0.5f),
        float2(-0.5f, -0.5f),
        float2( 0.5f, -0.5f),
    };

    VSOutput output;
    output.position = float4(positions[vertex_id], 0.0f, 1.0f);
    return output;
}
