/**
 * @file temperatures.cpp
 * @author leb
 * @brief 739. 每日温度
 * @version 0.1
 * @date 2023-10-10
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <iostream>
#include <stack>
#include <vector>
using namespace std;

class Solution {
 public:
  vector<int> dailyTemperatures(vector<int>& temperatures) {
    int size = temperatures.size();
    vector<int> v(size);
    stack<int> s;
    for (int i = 0; i < size; ++i) {
      while (!s.empty() && temperatures[i] > temperatures[s.top()]) {
        int temp = s.top();
        s.pop();
        v[temp] = i - temp;
      }
      s.push(i);
    }
    return v;
  }
};
int main() {
  vector<int> t = {73, 74, 75, 71, 69, 72, 76, 73};
  Solution s;
  auto it = s.dailyTemperatures(t);
  for (const auto& iter : it) {
    std::cout << iter << std::endl;
  }
}
