#ifndef SRC_NO_COPY_H_
#define SRC_NO_COPY_H_
namespace leb {
class NoCopy {
  protected:
    NoCopy(){}
    ~NoCopy(){}
  private:
    NoCopy(const NoCopy& copy);
    const NoCopy& operator=(const NoCopy&);
};
}
#endif