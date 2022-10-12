#include "timer.h"

using namespace std::chrono;

Timer::Timer()
{
    reset();
}

void Timer::reset()
{
    begin_ = high_resolution_clock::now();
}

double Timer::elapsed() const
{
    const auto end_ = high_resolution_clock::now();
    return duration_cast<seconds>(end_ - begin_).count();
}

double Timer::elapsedMs() const
{
    const auto end_ = high_resolution_clock::now();
    return duration_cast<milliseconds>(end_ - begin_).count();
}

double Timer::elapsedUs() const
{
    const auto end_ = high_resolution_clock::now();
    return duration_cast<microseconds>(end_ - begin_).count();
}
