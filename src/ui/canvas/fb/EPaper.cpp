// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EPaper.hpp"
#include "mxcfb.h"
#include "ui/canvas/memory/Export.hpp"

#include <sys/ioctl.h>

#include <cstring>

using std::chrono::steady_clock;

namespace {

/* Queueing faster than the panel shows frames only buries the newest
   frame behind stale ones, which is what makes a drag feel laggy. */
constexpr auto min_interval = std::chrono::milliseconds{120};

/* Bi-level waveforms leave residue; clean it up once drawing pauses
   (like xochitl's ~1s ghost cleanup), or after this many in a row. */
constexpr auto fast_cleanup_delay = std::chrono::seconds{1};
constexpr unsigned max_fast_updates = 12;

/* The panel is shared with other apps, so periodically re-assert all of
   it; xochitl does the same on a ~30s cooldown. */
constexpr auto full_refresh_interval = std::chrono::seconds{30};

/* DU is monochrome even on the Move's colour panel. Pixels are BGRA in
   memory order, so byte 0 is blue. */
constexpr bool
IsBiLevel(uint32_t pixel) noexcept
{
  const unsigned b = pixel & 0xff;
  const unsigned g = (pixel >> 8) & 0xff;
  const unsigned r = (pixel >> 16) & 0xff;

  return std::max({r, g, b}) <= 0x30 || std::min({r, g, b}) >= 0xd0;
}

} // namespace

void
EPaperPanel::SendUpdate(const Region &region, unsigned waveform) noexcept
{
  if (region.IsEmpty()) return;

  struct mxcfb_update_data update = {
      {
          uint32_t(region.top),
          uint32_t(region.left),
          uint32_t(region.right - region.left),
          uint32_t(region.bottom - region.top),
      },
      waveform,
      /* rm2fb treats FULL as "block until done" and forces a flashing GC16. */
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
EPaperPanel::SendQuality() noexcept
{
  const auto now = steady_clock::now();

  Region region = pending;
  region.Add(fast);

  /* GC16 flashes, but is the waveform that actually clears DU residue. */
  unsigned waveform = fast.IsEmpty() ? WAVEFORM_MODE_AUTO : WAVEFORM_MODE_GC16;

  if (force_full || now - last_full >= full_refresh_interval) {
    region = Region{0, 0, int(width), int(height)};
    waveform = WAVEFORM_MODE_GC16;
    last_full = now;
    force_full = false;
  }

  flush_timer.Cancel();
  SendUpdate(region, waveform);

  pending.Clear();
  pending_bi_level = true;
  fast.Clear();
  fast_updates = 0;
  last_update = now;
}

void
EPaperPanel::Flip(int _fd, void *map, unsigned pitch, unsigned bpp,
                  ConstImageBuffer<BGRAPixelTraits> src) noexcept
{
  fd = _fd;
  width = src.size.width;
  height = src.size.height;

  if (bpp != 4) {
    /* RGB565 (rM2): diffing would mean converting every pixel, so keep
       the plain copy-and-refresh-everything behaviour. */
    CopyFromBGRA(map, pitch, bpp, src);
    SendUpdate(Region{0, 0, int(width), int(height)}, WAVEFORM_MODE_AUTO);
    return;
  }

  /* At 32 bit the framebuffer holds the previous frame in the source's
     own layout, so compare against it while copying. */
  Region changed;
  bool bi_level = true;

  auto *dest_row = static_cast<uint8_t *>(map);
  const auto *src_row = reinterpret_cast<const uint8_t *>(src.data);

  for (unsigned y = 0; y < height;
       ++y, dest_row += pitch, src_row += src.pitch) {
    auto *dest = reinterpret_cast<uint32_t *>(dest_row);
    const auto *line = reinterpret_cast<const uint32_t *>(src_row);

    /* Most rows are unchanged; this is what makes the diff affordable. */
    if (std::memcmp(dest, line, size_t(width) * 4) == 0) continue;

    int x_min = int(width), x_max = -1;
    for (unsigned x = 0; x < width; ++x) {
      if (dest[x] == line[x]) continue;

      dest[x] = line[x];
      x_min = std::min(x_min, int(x));
      x_max = int(x);
      bi_level &= IsBiLevel(line[x]);
    }

    changed.Add(Region{x_min, int(y), x_max + 1, int(y) + 1});
  }

  if (changed.IsEmpty()) return;

  pending.Add(changed);
  pending_bi_level &= bi_level;

  const auto now = steady_clock::now();

  if (pending_bi_level && !force_full && fast_updates < max_fast_updates) {
    /* rm2fb turns DU+PARTIAL into FAST_DRAW, which doesn't queue behind
       other updates, so no rate limit here. */
    SendUpdate(pending, WAVEFORM_MODE_DU);
    fast.Add(pending);
    ++fast_updates;
    pending.Clear();
    pending_bi_level = true;
    last_update = now;
    flush_timer.Schedule(fast_cleanup_delay);
    return;
  }

  if (const auto elapsed = now - last_update; elapsed < min_interval) {
    /* Mid-burst: withhold, but make sure the last frame still lands. */
    flush_timer.Schedule(min_interval - elapsed);
    return;
  }

  SendQuality();
}

void
EPaperPanel::Flush() noexcept
{
  if (!pending.IsEmpty() || !fast.IsEmpty()) SendQuality();
}
