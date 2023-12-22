#include "open_hashtable.h"
#include <string>
class A {
 public:
  A(int a) : a_(a){}
 int a_;
};
int main() {

HashTable<int, A> table;
table.Init(1024, false);
A a(10);
A b (100);
table.Insert(10, &a);
table.Insert(20, &b);
table.UnInit();
return 0;
}