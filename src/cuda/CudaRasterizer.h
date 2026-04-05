#pragma once

#include <cstdint>

namespace OpenSplat {
namespace CUDA {

struct GeometryState {
    float* depths;
    int* radii;
    float* conic_opacity; // Was float3*, changed to float* for host compiler compatibility
    float* rgb;
    
    uint32_t* point_offsets;
    uint32_t* tiles_touched;
};

class CudaRasterizer {
public:
    // Memory Allocation functions
    static void AllocateBuffer(size_t num_gaussians);
    static void FreeBuffer();

    // -------------------------------------------------------------
    // FORWARD PASS
    // Projects Gaussians from 3D to 2D, sorts them by depth, and 
    // alpha-blends them into an image buffer.
    // -------------------------------------------------------------
    static void Forward(
        int P, // Number of Gaussians
        int width, int height,
        const float* background,
        const float* means3D,
        const float* shs,
        const float* colors_precomp,
        const float* opacities,
        const float* scales,
        const float* rotations,
        const float* view_matrix,
        const float* proj_matrix,
        const float* cam_pos,
        float* out_color // Output image
    );

    // -------------------------------------------------------------
    // BACKWARD PASS
    // Computes gradients with respect to 3D means, scales, rotations,
    // and opacities based on the loss gradient dL_dout_color
    // -------------------------------------------------------------
    static void Backward(
        int P, // Number of Gaussians
        int width, int height,
        const float* background,
        const float* means3D,
        const float* shs,
        const float* colors_precomp,
        const float* opacities,
        const float* scales,
        const float* rotations,
        const float* view_matrix,
        const float* proj_matrix,
        const float* cam_pos,
        const float* dL_dout_color, // Gradient of loss w.r.t rendered image
        float* dL_dmeans3D,         // Grad out: Means
        float* dL_dshs,             // Grad out: Spherical Harmonics
        float* dL_dscales,          // Grad out: Scales
        float* dL_drotations,       // Grad out: Rotations
        float* dL_dopacities        // Grad out: Opacities
    );
};

} // namespace CUDA
} // namespace OpenSplat
