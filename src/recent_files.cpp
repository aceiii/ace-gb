#include "recent_files.hpp"

namespace fs = std::filesystem;

RecentFiles::RecentFiles(size_t size): size_{size} {
  filenames_.reserve(size);
}

RecentFiles::const_iterator RecentFiles::begin() const {
  return filenames_.begin();
}

RecentFiles::const_iterator RecentFiles::end() const {
  return filenames_.end();
}

void RecentFiles::Pop() {
  if (filenames_.empty()) {
    return;
  }
  filenames_.erase(filenames_.begin());
}

void RecentFiles::Push(const fs::path& path) {
  fs::path abs_path = fs::absolute(path);
  auto it = std::find(filenames_.begin(), filenames_.end(), abs_path.string());
  if (it != filenames_.end()) {
    return;
  }

  if (filenames_.size() >= size_) {
    Pop();
  }

  filenames_.push_back(abs_path.string());
}

void RecentFiles::Remove(const fs::path& path) {
  fs::path abs_path = fs::absolute(path);
  std::erase(filenames_, abs_path.string());
}

void RecentFiles::Clear() {
  filenames_.clear();
}

bool RecentFiles::Contains(const fs::path& path) {
  fs::path abs_path = fs::absolute(path);
  auto it = std::find(filenames_.begin(), filenames_.end(), abs_path.string());
  return it != filenames_.end();
}

bool RecentFiles::IsEmpty() const {
  return filenames_.empty();
}
