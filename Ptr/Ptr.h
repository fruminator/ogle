#ifndef PTR_H
#define PTR_H

#include "ConstPtr.in"

template <class T>
class Ptr : public ConstPtr<T> {
  public:
    Ptr( T * ptr = 0 );
    Ptr( const Ptr<T>& mp );
    void operator=( const Ptr<T>& mp );
    const T * operator->() const { return rawPtr_; }
    T * operator->() { return (T *) rawPtr_; }
    T * rawPtr() const { return (T *) rawPtr_; }
    operator bool(){return rawPtr_? 1:0;}
  };

#endif /* PTR_H */




