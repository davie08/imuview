#ifndef IMUSO3_H
#define IMUSO3_H

#include "imu.h"

void IMUSO3Thread(struct imu_sample *imu_data, int* pitch, int* roll, int *yaw);

#endif

