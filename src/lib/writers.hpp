#pragma once
#include <ostream>
#include <chrono>
#include <algorithm>

namespace sandbox {

inline void add_timestamp(std::ostream& out, char const* buffer, int bytesRead) {
  auto now = std::chrono::high_resolution_clock::now();
  out << '[' << now << "] " << buffer;
}

inline void replace_commas_with_tabs(std::ostream& out, char* buffer, int bytesRead) {
  std::transform(buffer, buffer + bytesRead, buffer, [](char c) {
    return c == ',' ? '\t' : c;
  });
  out << buffer;
}

}  // end namespace sandbox
