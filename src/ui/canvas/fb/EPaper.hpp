// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/memory/Buffer.hpp"
#include "ui/canvas/memory/PixelTraits.hpp"
#include "ui/event/Timer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>

/**
 * Drives a reMarkable's e-paper panel through rm2fb's /dev/fb0 shim.
 *
 * XCSoar repaints its whole window on any change, so this diffs against
 * the framebuffer while copying into it, and refreshes only what changed,
 * with a waveform chosen from what the change looks like.
 */
class EPaperPanel {
  /** Half-open: right/bottom are one past the last pixel. */
  struct Region {
    int left = 0, top = 0, right = 0, bottom = 0;

    constexpr bool IsEmpty() const noexcept
    {
      return right <= left || bottom <= top;
    }

    void Clear() noexcept { *this = Region{}; }

    void Add(const Region &other) noexcept
    {
      if (other.IsEmpty()) return;
      if (IsEmpty()) {
        *this = other;
        return;
      }
      left = std::min(left, other.left);
      top = std::min(top, other.top);
      right = std::max(right, other.right);
      bottom = std::max(bottom, other.bottom);
    }
  };

  /** Changed, but not yet sent to the panel. */
  Region pending;
  bool pending_bi_level = true;

  /** Drawn with a bi-level waveform, so owed a GC16 cleanup. */
  Region fast;
  unsigned fast_updates = 0;

  uint32_t marker = 0;
  int fd = -1;
  unsigned width = 0, height = 0;

  std::chrono::steady_clock::time_point last_update{};
  std::chrono::steady_clock::time_point last_full{};

  /** Nothing of ours is on the panel yet. */
  bool force_full = true;

  /** Sends withheld updates and fast-update cleanup once drawing stops. */
  UI::Timer flush_timer{[this] { Flush(); }};

public:
  /** Copy into the mapped framebuffer and refresh whatever changed. */
  void Flip(int fd, void *map, unsigned pitch, unsigned bpp,
            ConstImageBuffer<BGRAPixelTraits> src) noexcept;

private:
  void Flush() noexcept;
  void SendUpdate(const Region &region, unsigned waveform) noexcept;
  void SendQuality() noexcept;
};
