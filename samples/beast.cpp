#include <vector>
#include <iostream>
#include <thread>
#include <algorithm>
#include <numeric>
#include <mutex>

using namespace std;

auto main(int argc, char *argv[]) -> int {

  std::cout << "I'm very hungry arrrr" << std::endl;

  std::vector<double> data(1'000'000);
  std::iota(data.begin(), data.end(), 0.0);

  std::mutex mtx;

  for (int iter = 0; iter < 10; iter++) {
    if (iter % 2 == 1) {
      auto t = std::thread([&] {
        std::unique_lock<std::mutex> lock(mtx);
        std::reverse(data.begin(), data.end());
      });
      {
        std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
        if (lock.try_lock()) {
          auto sum = std::accumulate(data.begin(), data.end(), 0.0);
          std::cout << "sum = " << sum << std::endl;
        } else {
          std::cerr << "Failed to lock" << std::endl;
        }
      }
      t.join();
    }
    else {
      std::unique_lock<std::mutex> lock(mtx);
      data.resize(data.size() * 2);
    }
  }

  // std::cout << "Data size = " << data.size()  << std::endl;
  size_t num_bytes = data.size()*sizeof(double);
  double num_kb = static_cast<double>(num_bytes)/1024.0;
  std::cout << "Usage should be at least " << num_kb << " KB" << std::endl;


  return 0;
}
