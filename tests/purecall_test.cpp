#include <stdio.h>

#include "../interfaces/ref.h"

int HarnessTestMode()
{
    return 1;
}

struct Base {
    Base();
    virtual void Pure() = 0;
};

struct Derived : Base {
    void Pure() {}
};

#pragma optimize("", off)
__declspec(noinline) void CallPureVirtual(Base* b)
{
    b->Pure();
}
#pragma optimize("", on)

Base::Base()
{
    CallPureVirtual(this);
}

int main()
{
    Derived d;
    printf("FAIL: the pure virtual call returned\n");
    return 1;
}
