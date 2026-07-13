// Builds up to 4 consecutive Hi-Z pyramid mip levels in one dispatch, each
// via the same 2x2 max reduction as a single-mip pass would use. Standard
// non-reversed depth (near=0, far=1, see clear_depth=1.0 throughout the
// engine), so the farthest (numerically largest) texel in each footprint is
// the conservative bound for occlusion tests.
//
// One 8x8 threadgroup computes an 8x8 tile of the first (finest) mip in the
// batch exactly as a single-mip dispatch would, then keeps reducing those
// same 64 values through groupshared memory to produce up to 3 more,
// progressively coarser mips (4x4, 2x2, 1x1) without a separate dispatch (and
// its barrier/pass-switch cost) per level. num_mips_this_dispatch (1-4) says
// how many of dst_mip0..dst_mip3 are real for this call - see
// SDL_GPUHiZPass::build for how batches are sized (mip_levels - 1 total
// transitions, up to 4 per dispatch).
//
// IMPORTANT: GroupMemoryBarrierWithGroupSync() requires every thread in the
// group to reach it - a thread that has already returned can't rendezvous
// with one that hasn't. num_mips_this_dispatch is uniform (same value for
// every thread, from the constant buffer), so branching on it to skip a
// later barrier entirely is safe (either all 64 threads take that branch, or
// none do). Branching on `local` (which thread within the group) is NOT
// safe to guard a barrier with - only the per-stage compute/write work is
// gated on `local`, never a barrier itself.
//
// Odd/undersized dimensions are handled ONCE, in stage 0 only: its source
// footprint clamps against src_size, and its destination coordinate clamps
// against dst_size0, so every one of the 64 local positions - including
// threads whose true position is beyond dst_size0 (group_count is sized to
// the finest mip's ceil(size/8), so the last group in each dimension can
// include such threads) - ends up holding a well-defined, correctly
// edge-duplicated value in shared_depth. Stages 1-3 read shared_depth purely
// by LOCAL thread index with fixed strides (1, 2, 4) that never leave the
// group's [0,7] range by construction, and never need to clamp against any
// mip's global dst_size again - doing so (an earlier, buggy version of this
// shader did) conflates local shared-memory indices with global texel
// coordinates and reads positions no earlier stage actually wrote,
// corrupting the reduction. The only per-stage use of a global dst_sizeN is
// the *write*-bounds check (`all(dst_xyN_raw < dst_sizeN)`), which is a
// different, still-necessary comparison of global coordinates.

RWTexture2D<float> src_mip : register(u0, space1);
RWTexture2D<float> dst_mip0 : register(u1, space1);
RWTexture2D<float> dst_mip1 : register(u2, space1);
RWTexture2D<float> dst_mip2 : register(u3, space1);
RWTexture2D<float> dst_mip3 : register(u4, space1);

cbuffer HiZDownsampleParams : register(b0, space2) {
    uint2 src_size;
    uint2 dst_size0;
    uint2 dst_size1;
    uint2 dst_size2;
    uint2 dst_size3;
    uint num_mips_this_dispatch;
    uint3 padding;
};

groupshared float shared_depth[8][8];

// 2x2 max reduction, with the caller responsible for clamping each corner's
// index against the source's own bounds before indexing - odd dimensions
// duplicate the last valid texel instead of reading garbage.
float reduce_2x2(
    float top_left, float top_right, float bottom_left, float bottom_right
) {
    return max(max(top_left, top_right), max(bottom_left, bottom_right));
}

[numthreads(8, 8, 1)]
void main(
    uint3 dispatch_thread_id : SV_DispatchThreadID,
    uint3 group_thread_id : SV_GroupThreadID
) {
    const uint2 local = group_thread_id.xy;
    const uint2 dst_xy0_raw = dispatch_thread_id.xy;

    // Stage 0: src_mip -> dst_mip0. All 64 threads always participate.
    const uint2 dst_max0 = dst_size0 - uint2(1, 1);
    const uint2 dst_xy0 = min(dst_xy0_raw, dst_max0);
    const uint2 src_max = src_size - uint2(1, 1);
    const uint2 base0 = dst_xy0 * 2;
    const float v0 = reduce_2x2(
        src_mip[min(base0, src_max)],
        src_mip[min(base0 + uint2(1, 0), src_max)],
        src_mip[min(base0 + uint2(0, 1), src_max)],
        src_mip[min(base0 + uint2(1, 1), src_max)]
    );

    shared_depth[local.y][local.x] = v0;
    if (all(dst_xy0_raw < dst_size0)) {
        dst_mip0[dst_xy0_raw] = v0;
    }

    // num_mips_this_dispatch is uniform, so every "if" gating a barrier
    // below is taken identically by all 64 threads - never guarded by
    // `local`.
    if (num_mips_this_dispatch >= 2) {
        GroupMemoryBarrierWithGroupSync();

        // Stage 1: mip0 (in shared_depth) -> dst_mip1. Only the 16 threads
        // at even local indices do real work here; the rest do nothing but
        // still participate in every barrier from here on.
        // No clamping needed here (or in stages 2/3 below): stage 0 already
        // edge-duplicated every one of the 64 local positions against the
        // real image bounds (dst_max0/src_max above), so every local index
        // this stage reads - always within [0,7], never past it, by
        // construction of the fixed 8x8 group and power-of-2 stride - already
        // holds a correct, possibly-duplicated value. Clamping again here
        // against the GLOBAL mip size (as an earlier version of this shader
        // did) is a bug: it conflates local shared-memory indices with
        // global texel coordinates and reads positions no earlier stage
        // actually wrote.
        const bool is_stage1_active = (local.x % 2 == 0) && (local.y % 2 == 0);
        float v1 = 0.0f;
        if (is_stage1_active) {
            v1 = reduce_2x2(
                shared_depth[local.y][local.x],
                shared_depth[local.y][local.x + 1],
                shared_depth[local.y + 1][local.x],
                shared_depth[local.y + 1][local.x + 1]
            );

            const uint2 dst_xy1_raw = dst_xy0_raw / 2;
            if (all(dst_xy1_raw < dst_size1)) {
                dst_mip1[dst_xy1_raw] = v1;
            }
        }

        if (num_mips_this_dispatch >= 3) {
            GroupMemoryBarrierWithGroupSync();
            if (is_stage1_active) {
                shared_depth[local.y][local.x] = v1;
            }
            GroupMemoryBarrierWithGroupSync();

            // Stage 2: mip1 (in shared_depth, at even-even local indices) ->
            // dst_mip2. Only 4 threads do real work.
            const bool is_stage2_active = (local.x % 4 == 0) && (local.y % 4 == 0);
            float v2 = 0.0f;
            if (is_stage2_active) {
                v2 = reduce_2x2(
                    shared_depth[local.y][local.x],
                    shared_depth[local.y][local.x + 2],
                    shared_depth[local.y + 2][local.x],
                    shared_depth[local.y + 2][local.x + 2]
                );

                const uint2 dst_xy2_raw = dst_xy0_raw / 4;
                if (all(dst_xy2_raw < dst_size2)) {
                    dst_mip2[dst_xy2_raw] = v2;
                }
            }

            if (num_mips_this_dispatch >= 4) {
                GroupMemoryBarrierWithGroupSync();
                if (is_stage2_active) {
                    shared_depth[local.y][local.x] = v2;
                }
                GroupMemoryBarrierWithGroupSync();

                // Stage 3: mip2 (in shared_depth, at multiples-of-4 local
                // indices) -> dst_mip3. Only the single thread at (0,0)
                // does real work.
                if (local.x == 0 && local.y == 0) {
                    const float v3 = reduce_2x2(
                        shared_depth[0][0],
                        shared_depth[0][4],
                        shared_depth[4][0],
                        shared_depth[4][4]
                    );

                    const uint2 dst_xy3_raw = dst_xy0_raw / 8;
                    if (all(dst_xy3_raw < dst_size3)) {
                        dst_mip3[dst_xy3_raw] = v3;
                    }
                }
            }
        }
    }
}
