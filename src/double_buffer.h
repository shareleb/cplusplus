#ifndef SRC_DOUBLE_BUFFER_H_
#define SRC_DOUBLE_BUFFER_H_
#include <vector>
#include <atomic>
template<class T>
class DoubleBuffer {

public:
    template<class...Ts>
    DoubleBuffer(Ts&&...args):
        index_(0),
        value_0_(std::forward<Ts>(args)...m)

    {
        
    } 


private:
  std::vector<T*> buff_;
  T val0_;
  T val1_;
  std::atomic<uint32_t> index_;

};


#endif
