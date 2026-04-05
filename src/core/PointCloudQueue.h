#pragma once
#include <atomic>
#include <vector>
#include <glm/glm.hpp>
#include "core/Gaussian.h"

namespace OpenSplat {
namespace Core {

// SPSC non-blocking queue — slot count = 2 (double-buffered)
class PointCloudQueue {
public:
    bool TryPush(std::vector<Gaussian>&& pts) {
        int readIdx = m_ReadIdx.load(std::memory_order_acquire);
        int writeIdx = m_WriteIdx.load(std::memory_order_relaxed);
        int nextWriteIdx = (writeIdx + 1) % kSlots;
        
        if (nextWriteIdx == readIdx) {
            return false; // Queue is full
        }
        
        m_Slots[writeIdx] = std::move(pts);
        m_WriteIdx.store(nextWriteIdx, std::memory_order_release);
        return true;
    }
    
    bool TryPop(std::vector<Gaussian>& out) {
        int readIdx = m_ReadIdx.load(std::memory_order_relaxed);
        int writeIdx = m_WriteIdx.load(std::memory_order_acquire);
        
        if (readIdx == writeIdx) {
            return false; // Queue is empty
        }
        
        out = std::move(m_Slots[readIdx]);
        
        int nextReadIdx = (readIdx + 1) % kSlots;
        m_ReadIdx.store(nextReadIdx, std::memory_order_release);
        return true;
    }
    
private:
    static constexpr int kSlots = 2;
    std::vector<Gaussian> m_Slots[kSlots];
    std::atomic<int> m_WriteIdx{0}; // written by producer
    std::atomic<int> m_ReadIdx{0};  // written by consumer
};

} // namespace Core
} // namespace OpenSplat
