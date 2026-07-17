///
/// PackManager — owns and loads all .PAK game archives
///
#pragma once

#include <array>
#include <functional>
#include <string>
#include <string_view>

#include "core/lz_uty.h"

class PackArchive {
public:
  PackArchive() = default;
  PackArchive(std::string_view filename,
              std::function<bool(const PACKFILE_READ &)> on_load = {})
      : filename_(filename), on_load_(std::move(on_load)) {}

  bool Load(std::string_view data_path);

  const PACKFILE_READ &Get() const { return data_; }
  const std::string &Path() const { return filename_; }

private:
  PACKFILE_READ data_;
  std::string filename_;
  std::function<bool(const PACKFILE_READ &)> on_load_;
};

class PackManager {
public:
  bool LoadAll();

  const PACKFILE_READ &Map()    const { return packs_[kMap].Get(); }
  const PACKFILE_READ &Images() const { return packs_[kImages].Get(); }
  const PACKFILE_READ &Music()  const { return packs_[kMusic].Get(); }
  const PACKFILE_READ &Sound()  const { return packs_[kSound].Get(); }

  std::string MissingFilesReport() const;

private:
  static constexpr size_t kMap    = 0;
  static constexpr size_t kImages = 1;
  static constexpr size_t kMusic  = 2;
  static constexpr size_t kSound  = 3;

  std::array<PackArchive, 4> packs_;
};

inline PackManager packs;
