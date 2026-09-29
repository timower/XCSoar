// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EPaper.hpp"
#include "mxcfb.h"
#include "ui/canvas/memory/Export.hpp"

#include <sys/ioctl.h>

#include <algorithm>

using std::chrono::steady_clock;

namespace {

/* DU or A2: both skip the flash; A2 is faster but dithers to mono. */
constexpr unsigned fast_waveform = WAVEFORM_MODE_DU;

/* Clears fast-waveform ghosting; like xochitl's ~30s ghost cleanup. */
constexpr auto full_refresh_interval = std::chrono::seconds{30};

/* A due full refresh waits this long without drawing, so it never
   interrupts a pan; short enough to fit between 1 Hz GPS redraws. */
constexpr auto idle_delay = std::chrono::milliseconds{500};

} // namespace

void
EPaperPanel::SendUpdate(unsigned waveform) noexcept
{
  struct mxcfb_update_data update = {
      {0, 0, width, height},
      waveform,
      /* rm2fb treats FULL as "block until done". */
      UPDATE_MODE_PARTIAL,
      ++marker,
      TEMP_USE_AMBIENT,
      0,
      0,
      0,
  };

  ioctl(fd, MXCFB_SEND_UPDATE, &update);
}

void
EPaperPanel::Flip(int _fd, void *map, unsigned pitch, unsigned bpp,
                  ConstImageBuffer<BGRAPixelTraits> src) noexcept
{
  fd = _fd;
  width = src.size.width;
  height = src.size.height;

  CopyFromBGRA(map, pitch, bpp, src);
  SendUpdate(fast_waveform);

  const auto now = steady_clock::now();
  last_update = now;

  const auto due = last_full + full_refresh_interval;
  refresh_timer.Schedule(
      std::max<steady_clock::duration>(due - now, idle_delay));
}

void
EPaperPanel::OnRefreshTimer() noexcept
{
  const auto now = steady_clock::now();
  if (const auto idle = now - last_update; idle < idle_delay) {
    refresh_timer.Schedule(idle_delay - idle);
    return;
  }

  SendUpdate(WAVEFORM_MODE_GC16);
  last_full = now;
}
