#ifndef INTERFACE_H
#define INTERFACE_H

#include "stdio.h"

typedef unsigned long RefCount;

class Interface {
  public:

    Interface() : references_( 0 ) { }

    RefCount references() const { return references_; }

    void newRef() const {
        Interface * me = (Interface *) this;
        ++me->references_;
    }

    void deleteRef() const {
        Interface * me = (Interface *) this;
        if ( --me->references_ == 0 ) me->onZeroReferences();
    }

  protected:
    virtual ~Interface() {}

  private:
    virtual void onZeroReferences() { delete this; }
    RefCount references_;
};

    

#endif /* INTERFACE_H */
