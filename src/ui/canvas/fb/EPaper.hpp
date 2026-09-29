// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/memory/Buffer.hpp"
#include "ui/canvas/memory/PixelTraits.hpp"
#include "ui/event/Timer.hpp"

#include <chrono>
#include <cstdint>

/**
 * Drives a reMarkable's e-paper panel through rm2fb's /dev/fb0 shim.
 *
 * Every frame goes out on a fast waveform; the ghosting that leaves is
 * cleared by a periodic full refresh, held back while drawing continues.
 */
class EPaperPanel {
  uint32_t marker = 0;
  int fd = -1;
  unsigned width = 0, height = 0;

  std::chrono::steady_clock::time_point last_update{};
  std::chrono::steady_clock::time_point last_full{};

  UI::Timer refresh_timer{[this] { OnRefreshTimer(); }};

public:
  /** Copy into the mapped framebuffer and refresh the panel. */
  void Flip(int fd, void *map, unsigned pitch, unsigned bpp,
            ConstImageBuffer<BGRAPixelTraits> src) noexcept;

private:
  void OnRefreshTimer() noexcept;
  void SendUpdate(unsigned waveform) noexcept;
};
