#include <nanobind/nanobind.h>

int add(int a, int b)
{
    return a + b;
}

NB_MODULE(autocookie, b)
{
    b.def("add", &add);
}