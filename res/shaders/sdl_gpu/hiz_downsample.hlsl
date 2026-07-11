// Builds one Hi-Z pyramid mip level from the previous one via a 2x2 max
// reduction. Standard non-reversed depth (near=0, far=1, see clear_depth=1.0
// throughout the engine), so the farthest (numerically largest) texel in
// each footprint is the conservative bound for occlusion tests. Odd source
// dimensions are handled by clamping footprint corners to the last valid
// texel. Both mips are bound as read-write storage textures (u0 = source,
// u1 = destination) since SDL_GPU's read-only storage-texture binding has no
// per-mip selection; u0 is only ever read here.

RWTexture2D<float> src_mip : register(u0, space1);
RWTexture2D<float> dst_mip : register(u1, space1);

cbuffer HiZDownsampleParams : register(b0, space2) {
    uint2 dst_size;
    uint2 src_size;
};

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    const uint2 dst_xy = dispatch_thread_id.xy;
    if (dst_xy.x >= dst_size.x || dst_xy.y >= dst_size.y) {
        return;
    }

    const uint2 src_max = src_size - uint2(1, 1);
    const uint2 base = dst_xy * 2;

    const uint2 xy0 = min(base, src_max);
    const uint2 xy1 = min(base + uint2(1, 0), src_max);
    const uint2 xy2 = min(base + uint2(0, 1), src_max);
    const uint2 xy3 = min(base + uint2(1, 1), src_max);

    const float depth = max(
        max(src_mip[xy0], src_mip[xy1]), max(src_mip[xy2], src_mip[xy3])
    );

    dst_mip[dst_xy] = depth;
}
