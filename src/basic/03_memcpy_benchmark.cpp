#include "../utils/timer.h"
#include <cstdio>
#include <vector>
#include <cstring>

int main()
{
    int size = 1024 * 1024 * 8;
    std::vector<uint8_t> src(size);
    std::vector<uint8_t> dst(size);

    for(int i = 0; i < size; i++)
        src[i] = rand() % 256;

    Timer timer;
    for(int i = 0; i < 10; i++)
    {
        timer.reset();
        std::memcpy(dst.data(), src.data(), size);
        printf("time elapsed: %fms\n", timer.elapsedUs() / 1000);
    }

    return 0;
}