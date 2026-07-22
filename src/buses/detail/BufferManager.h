#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/Compat.h"
#include "core/Pixel.h"

namespace lw::buses::detail
{

// ---------------------------------------------------------------------------
// BufferManager — owns protocol buffer(s) and an optional scratch pixel buffer.
//
// Protocol buffer sizes are computed externally (from the concrete protocol
// type's static requiredBufferSize()) and passed to the constructor. The
// scratch pixel buffer is allocated only when needsScratch is true.
//
// Supports both single-run (one protocol buffer) and multi-run (one protocol
// buffer per run, shared scratch) configurations.
// ---------------------------------------------------------------------------

class BufferManager
{
public:
  // -----------------------------------------------------------------------
  // Single run
  // -----------------------------------------------------------------------

  /// @param protocolBufferSize  Size in bytes for the protocol buffer.
  /// @param pixelCount          Number of pixels in the strip (for scratch sizing).
  /// @param needsScratch        Whether to allocate a scratch pixel buffer.
  BufferManager(size_t protocolBufferSize, size_t pixelCount, bool needsScratch) : _protocolBuffers(1), _scratchPixels(needsScratch ? pixelCount : 0) { _protocolBuffers[0].resize(protocolBufferSize); }

  // -----------------------------------------------------------------------
  // Multi run
  // -----------------------------------------------------------------------

  /// @param protocolBufferSizes  One size per run.
  /// @param pixelCount           Total pixel count (for scratch sizing).
  /// @param needsScratch         Whether to allocate a scratch pixel buffer.
  BufferManager(lw::span<const size_t> protocolBufferSizes, size_t pixelCount, bool needsScratch) : _protocolBuffers(protocolBufferSizes.size()), _scratchPixels(needsScratch ? pixelCount : 0)
  {
    for (size_t i = 0; i < protocolBufferSizes.size(); ++i)
    {
      _protocolBuffers[i].resize(protocolBufferSizes[i]);
    }
  }

  // -----------------------------------------------------------------------
  // Accessors
  // -----------------------------------------------------------------------

  /// Number of protocol buffers (= number of runs).
  size_t runCount() const { return _protocolBuffers.size(); }

  /// True when a scratch pixel buffer was allocated.
  bool hasScratch() const { return !_scratchPixels.empty(); }

  /// Protocol buffer for a given run. Default run index 0 for single-run.
  lw::span<uint8_t> protocolBuffer(size_t runIndex = 0) { return lw::span<uint8_t>{_protocolBuffers[runIndex].data(), _protocolBuffers[runIndex].size()}; }

  lw::span<const uint8_t> protocolBuffer(size_t runIndex = 0) const { return lw::span<const uint8_t>{_protocolBuffers[runIndex].data(), _protocolBuffers[runIndex].size()}; }

  /// Scratch pixel buffer. Empty span when scratch was not allocated.
  lw::span<lw::Pixel> scratchPixels() { return lw::span<lw::Pixel>{_scratchPixels.data(), _scratchPixels.size()}; }

  lw::span<const lw::Pixel> scratchPixels() const { return lw::span<const lw::Pixel>{_scratchPixels.data(), _scratchPixels.size()}; }

private:
  std::vector<std::vector<uint8_t>> _protocolBuffers;
  std::vector<lw::Pixel> _scratchPixels;
};

} // namespace lw::buses::detail
