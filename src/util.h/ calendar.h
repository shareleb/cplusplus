#ifndef SRC_UTIL_CALENDAR_H_
#define SRC_UTIL_CALENDAR_H_
#include <string>
namespace leb {

//1971-2100区间, 根据输入年月日，返回是星期几。
std::string CalculateWeek(int year, int month, int day);
}
#endif 