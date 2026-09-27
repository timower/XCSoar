// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifndef REMARKABLE
#error This header is only for reMarkable builds
#endif

enum class RemarkableModel {
  UNKNOWN,
  RM1,
  RM2,

  /** reMarkable Paper Pro ("Ferrari") */
  PAPER_PRO,

  /** reMarkable Paper Pro Move ("Chiappa") */
  PAPER_PRO_MOVE,

  /** reMarkable Paper Pure ("Tatsu") */
  PAPER_PURE,
};

[[gnu::const]]
RemarkableModel
DetectRemarkableModel() noexcept;

