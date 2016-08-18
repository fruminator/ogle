#ifndef CONSTPTR_H
#define CONSTPTR_H

template <class T>
class ConstPtr {
  public:
    ConstPtr( const T * ptr = 0 );
    ConstPtr( const ConstPtr<T>& mp );
    ~ConstPtr();
    const T * operator->() const { return rawPtr_; }
    const T * rawPtr() const { return rawPtr_; }

  protected:
    void operator=( const ConstPtr<T>& mp );
    const T * rawPtr_;
  };

#endif /* CONSTPTR_H */
