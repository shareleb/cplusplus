#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace leb {

template <class C>
class DoubleBuff {
 public:
  DoubleBuff() : index_(0) {}

  std::shared_ptr<C> get() const { return buffer_[index_]; }

  void set(std::shared_ptr<C> data) {
    std::lock_guard<std::mutex> guard(lock_);
    size_t update_index = (index_ + 1) % 2;
    buffer_[update_index] = data;
    index_ = update_index;
  }

 private:
  std::shared_ptr<C> buffer_[2];
  std::atomic<size_t> index_;
  std::mutex lock_;
};

}  
