#include "mult_thread/use_thread.h"
#include <iostream>
#include <string>
#include <thread>

namespace leb {

void func(int num) {
    for (int i = 0; i < num; i++) {
        std::cout << i << std::endl;
    }
}
void test_thread() {
  std::thread t1(func, 10);
  t1.join();
}
}