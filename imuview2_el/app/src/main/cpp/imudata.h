//
// Created by Liio Chen on 2022/1/5.
//

#ifndef IMUVIEW_IMUDATA_H
#define IMUVIEW_IMUDATA_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#define IMU_DEFAULT_HZ      20

#define IMU_ARRAY_SIZE      6

struct imu_sample {
    uint64_t time_us;
    short data[IMU_ARRAY_SIZE];
};

static inline uint64_t seconds_to_nanoseconds(uint64_t secs)
{
return secs*1000000000;
}

static inline uint64_t milliseconds_to_nanoseconds(uint64_t secs)
{
return secs*1000000;
}

static inline uint64_t microseconds_to_nanoseconds(uint64_t secs)
{
return secs*1000;
}

static inline uint64_t nanoseconds_to_seconds(uint64_t secs)
{
return secs/1000000000;
}

static  inline uint64_t nanoseconds_to_milliseconds(uint64_t secs)
{
return secs/1000000;
}

static  inline uint64_t nanoseconds_to_microseconds(uint64_t secs)
{
return secs/1000;
}

static __inline uint64_t getSystemTime() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t) now.tv_sec*1000000000LL + now.tv_nsec;
}

extern RingBuffer<imu_sample> *imu_buffer;
extern struct imu_sample calibration_data;

#endif //IMUVIEW_IMUDATA_H
