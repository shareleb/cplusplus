#ifndef SRC_LOCK_H_
#define SRC_LOCK_H_
#include <pthread.h>
#include "NoCopy.h"
namespace leb
{
class Lock : public NoCopy {
 public:
  Lock() {
    pthread_mutex_init(&lock_, NULL);
  }
  ~Lock() {
    pthread_mutex_destroy(&lock_);
  }
  void lock() const {
    pthread_mutex_lock(&lock_);
  }
  void unlock() const {
    pthread_mutex_unlock(&lock_);
  }

 private:
   mutable pthread_mutex_t lock_;
};

class LockGuard : public NoCopy {
 public:
  explicit LockGuard(Lock& lock) : lock_(lock) {
    lock_.lock();
  }
  ~LockGuard() {
    lock_.unlock();
  }
 private:
  Lock& lock_;
};
    
} // namespace leb

#endif