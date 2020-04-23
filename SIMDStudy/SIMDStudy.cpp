// SIMD study inspired by "GDC 2015 - SIMD at Insomniac Games"

#include "test_aos.h"
#include "test_soa.h"
#include <iostream>

int main()
{
    float aos, soa, sse, avx;
    test_aos(aos);
    test_soa(soa, sse, avx);

    std::cout << "aos/soa = " << (aos / soa) << std::endl;
    std::cout << "aos/sse = " << (aos / sse) << std::endl;
    std::cout << "aos/avx = " << (aos / avx) << std::endl;

    return 0;
}
