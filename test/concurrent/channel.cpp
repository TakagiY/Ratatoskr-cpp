#include "../../ratatoskr/concurrent.hpp"
#include "../test.hpp"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

void one_vs_one() {
  auto one_vs_one_test = [](auto input) {
    auto [sn, rc] = rat::make_channel<typename decltype(input)::value_type>();
    decltype(input) output;

    auto consume = [&output](auto rc) {
      try {
        while (true) {
          output.push_back(rc.next());
        }
      }
      catch (const rat::close_channel &) {
      }
    };

    std::thread consumer{consume, std::move(rc)};

    for (auto &&elem : input) {
      sn.push(elem);
    }
    std::this_thread::sleep_for(0.1s);
    sn.close();
    consumer.join();

    return output;
  };

  std::vector input = {1,  2,  4, 5, 6, 7, 6, 7, 6,    4, 3,  52, 256, 2,  52,
                       44, 24, 1, 3, 2, 4, 1, 1, 4131, 4, 13, 21, 1,   34, 3};

  test::check(one_vs_one_test, input, input);
}

void one_vs_n() {

  auto one_vs_n_test = [](auto input) {
    auto [sn, rc] = rat::make_channel<typename decltype(input)::value_type>(
        rat::sharing_receiver);

    std::mutex mutex_for_output;
    decltype(input) output;
    std::vector<std::thread> consumers;

    for (int i = 0; i < 10; ++i) {
      consumers.emplace_back(
          [&output, &mutex_for_output](auto rc) {
            try {
              while (true) {
                auto next = rc->next();
                {
                  std::lock_guard lock{mutex_for_output};
                  output.push_back(next);
                }
              }
            }
            catch (const rat::close_channel &) {
            }
          },
          rc);
    }

    for (auto &&elem : input) {
      sn.push(elem);
    }
    std::this_thread::sleep_for(0.1s);
    sn.close();

    for (auto &&t : consumers) {
      t.join();
    }
    std::sort(output.begin(), output.end());
    return output;
  };

  std::vector input = {1,  2,  4, 5, 6, 7, 6, 7, 6,    4, 3,  52, 256, 2,  52,
                       44, 24, 1, 3, 2, 4, 1, 1, 4131, 4, 13, 21, 1,   34, 3};

  std::vector expected = [](auto input) {
    std::sort(input.begin(), input.end());
    return input;
  }(input);

  test::check(one_vs_n_test, input, expected);
}

int main() {
  one_vs_one();
  one_vs_n();
}
