#pragma once

#include <filesystem>
#include <vector>



class RecentFiles {
public:
  using const_iterator = std::vector<std::string>::const_iterator;

  explicit RecentFiles(size_t size);
  const_iterator begin() const;
  const_iterator end() const;
  void Pop();
  void Push(const std::filesystem::path& path);
  void Remove(const std::filesystem::path& path);
  void Clear();
  bool Contains(const std::filesystem::path& path);
  bool IsEmpty() const;

private:
  size_t size_;
  std::vector<std::string> filenames_;
};

