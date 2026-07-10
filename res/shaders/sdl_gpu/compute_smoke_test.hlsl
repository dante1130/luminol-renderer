// Throwaway validation shader for the SDL_GPU compute pipeline plumbing
// (see Demo/ComputeSmokeTest). Writes each thread's dispatch index into a
// read-write storage buffer so the CPU can read it back and confirm the
// full HLSL -> SPIR-V -> compute-pipeline -> dispatch -> readback path works.

RWStructuredBuffer<uint> output_buffer : register(u0, space1);

[numthreads(64, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    output_buffer[dispatch_thread_id.x] = dispatch_thread_id.x;
}
