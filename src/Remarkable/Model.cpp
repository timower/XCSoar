// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Model.hpp"
#include "system/FileUtil.hpp"
#include "util/StringCompare.hxx"

#include <string.h>

static constexpr struct {
  const char *machine;
  RemarkableModel model;
} remarkable_model_ids[] = {
  { "reMarkable 2.0", RemarkableModel::RM2 },
  { "reMarkable Ferrari", RemarkableModel::PAPER_PRO },
  { "reMarkable Chiappa", RemarkableModel::PAPER_PRO_MOVE },
  { "reMarkable Tatsu", RemarkableModel::PAPER_PURE },
};

RemarkableModel
DetectRemarkableModel() noexcept
{
  char machine[64];
  if (!File::ReadString(Path("/sys/devices/soc0/machine"),
                        machine, sizeof(machine)))
    return RemarkableModel::UNKNOWN;

  for (const auto &i : remarkable_model_ids)
    if (StringStartsWith(machine, i.machine))
      return i.model;

  /* the reMarkable 1 is the only one that doesn't name itself
     "reMarkable <something>" */
  return StringStartsWith(machine, "reMarkable")
    ? RemarkableModel::UNKNOWN
    : RemarkableModel::RM1;
}
