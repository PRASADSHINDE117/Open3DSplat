#ifdef __CUDACC__
#pragma nv_diag_suppress 20012
#pragma nv_diag_suppress 550
#endif

#include "cuda/CudaRasterizer.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <iostream>

namespace OpenSplat {
namespace CUDA {

#define CHECK_CUDA(call) \
do { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
        std::cerr << "CUDA Error at " << __FILE__ << ":" << __LINE__ << " - " << cudaGetErrorString(err) << std::endl; \
        exit(EXIT_FAILURE); \
    } \
} while(0)


// ----------------------------------------------
// STUB FORWARD KERNEL
// ----------------------------------------------
__global__ void preprocess_forward_kernel(int P, const float* means3D, float* depths) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= P) return;
    // Example math: projecting 3D point to camera space
    depths[idx] = 1.0f; // stub
}

// ----------------------------------------------
// API
// ----------------------------------------------
void CudaRasterizer::AllocateBuffer(size_t num_gaussians) {
    // cudaMalloc...
}

void CudaRasterizer::FreeBuffer() {
    // cudaFree...
}

void CudaRasterizer::Forward(
        int P, int width, int height,
        const float* background, const float* means3D, const float* shs, const float* colors_precomp,
        const float* opacities, const float* scales, const float* rotations,
        const float* view_matrix, const float* proj_matrix, const float* cam_pos, float* out_color) 
{
    // 1. Grid / Block Setup
    int threads = 256;
    int blocks = (P + threads - 1) / threads;
    (void)blocks; // Suppress unused-variable warning until kernel launches are implemented

    // 2. Launch Preprocess Kernel (Frustum culling, cov3D -> cov2D, depth comp)
    // preprocess_forward_kernel<<<blocks, threads>>>(...);
    // CHECK_CUDA(cudaDeviceSynchronize());

    // 3. Launch Radix Sort (e.g. cub::DeviceRadixSort) based on computed depths

    // 4. Launch Render Kernel (Tile-based alpha compositing)
    // render_forward_kernel<<<...>>>(...);

    std::cout << "[CUDA] Executed Forward Rasterization pass for " << P << " Gaussians." << std::endl;
}


void CudaRasterizer::Backward(
        int P, int width, int height,
        const float* background, const float* means3D, const float* shs, const float* colors_precomp,
        const float* opacities, const float* scales, const float* rotations,
        const float* view_matrix, const float* proj_matrix, const float* cam_pos,
        const float* dL_dout_color, float* dL_dmeans3D, float* dL_dshs, float* dL_dscales, 
        float* dL_drotations, float* dL_dopacities) 
{
    // Launch Render Backward Kernel (compute gradients w.r.t 2D params)

    // Launch Preprocess Backward Kernel (compute gradients w.r.t 3D params)
    
    std::cout << "[CUDA] Executed Backward Rasterization pass." << std::endl;
}

} // namespace CUDA
} // namespace OpenSplat
