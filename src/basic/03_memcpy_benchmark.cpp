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
    int N = 10;
    float avg_time = 0;
    for(int i = 0; i < N; i++)
    {
        timer.reset();
        std::memcpy(dst.data(), src.data(), size);
        printf("time elapsed: %fms\n", timer.elapsedUs() / 1000);
        avg_time += timer.elapsedUs() / 1000;
    }
    avg_time /= N;

    printf("time per MB: %fms\n", avg_time / size * 1e6);

    return 0;
}