#ifndef DC6E8BF3_884F_4F04_8193_C89A5CA5A067
#define DC6E8BF3_884F_4F04_8193_C89A5CA5A067

#include <chrono>
#include <iostream>

class Timer {
public:
    Timer();
    void reset();
    double elapsed() const;    // in seconds
    double elapsedMs() const;  // in milliseconds
    double elapsedUs() const;  // in microseconds

private:
    std::chrono::high_resolution_clock::time_point begin_;
};

#endif /* DC6E8BF3_884F_4F04_8193_C89A5CA5A067 */
