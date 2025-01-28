#pragma once
#include <ostream>
#include <chrono>
#include <algorithm>

namespace sandbox {

/* This file contains the functions that can be executed by process monitor.
 * They modify the input before posting data into the output stream.
 */

// This function prepends the buffer content with high-precision timestamps
inline void add_timestamp(std::ostream& out, char const* buffer, int bytesRead) {
  auto now = std::chrono::high_resolution_clock::now();
  out << '[' << now << "] " << buffer;
}

// this function modifies the buffer by replacign its ',' symbols with tabs '\t'
// and posts the modified buffer into the output stream
inline void replace_commas_with_tabs(std::ostream& out, char* buffer, int bytesRead) {
  std::transform(buffer, buffer + bytesRead, buffer, [](char c) {
    return c == ',' ? '\t' : c;
  });
  out << buffer;
}

}  // end namespace sandbox
