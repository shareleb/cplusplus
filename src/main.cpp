#include <Lock.h>

namespace leb
{
int main() {
  const Lock lock;
  lock.lock();

  return 0;
}
    
} // namespace leb

