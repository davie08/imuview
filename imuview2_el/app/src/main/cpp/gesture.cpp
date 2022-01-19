//
// Created by daviepeng on 2022/1/10.
//
#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include "math.h"
#include "RingBuffer.h"
#include "imudata.h"

#define LOG_TAG "IMUDATA_NATIVE"

#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static uint64_t startAction = 0;
static int prePitch, preRoll, preYaw;
static int gestureOK = 0;
static int stopCapture = 0;
static int gestureOK_wait = 0;

// 手势最小间隔时间
#define GESTURE_INTERVAL_TIME           500*1000

// 一个手势判断的单位时间
#define GESTURE_DOING_TIME              80*1000

// 判断手势有效的移动值
#define GESTURE_THRESHOLD_ANGLE_VALUE           6

// 定义没有动作时的判断阀值
#define GESTURE_STATIC_THRESHOLD_ANGLE_VALUE    2

int is_stop_gesture(struct imu_sample *sample, int pitch, int roll, int yaw)
{
    return 0;

    //LOGD("%d", sample->data[1]);

    if (abs(sample->data[1]) > 1900) {
        return 1;
    }

    return 0;
}

int process_gesture(struct imu_sample *sample, int pitch, int roll, int yaw)
{
    int ret = 0;

    //LOGD("%05d %05d %05d   %05d %05d %05d", \
    //                        sample->data[0], sample->data[1], sample->data[2], \
    //                        sample->data[3], sample->data[4], sample->data[5]
    //     );

    if (is_stop_gesture(sample, pitch, roll, yaw)) {
        startAction = 0;
        gestureOK = 0;

        preYaw = 0;
        prePitch = 0;
        preRoll = 0;

        if (stopCapture == 0) {
            LOGD("stop gesture capture");
        }

        stopCapture = 1;
        return -1;
    }

    stopCapture = 0;

    if (preYaw == 0) {
        preYaw = yaw;
        prePitch = pitch;
        preRoll = roll;
        return -1;
    }

    if (gestureOK == 1) {

        if (abs(prePitch - pitch) > GESTURE_STATIC_THRESHOLD_ANGLE_VALUE || \
            abs(preRoll - roll) > GESTURE_STATIC_THRESHOLD_ANGLE_VALUE || \
            abs(preYaw - yaw) > GESTURE_STATIC_THRESHOLD_ANGLE_VALUE) {
            preYaw = yaw;
            prePitch = pitch;
            preRoll = roll;

            gestureOK_wait = 0;
            return -1;
        }
        else {
            gestureOK_wait++;

            // static 状态计数 > 10，每一帧是20ms，即200ms左右。

            if (gestureOK_wait > 5)
            {
                startAction = 0;
                gestureOK = 0;

                preYaw = 0;
                prePitch = 0;
                preRoll = 0;

                return -1;
            }

            return -1;
        }
    }

    if (startAction != 0) {
        if (abs(preYaw - yaw) > GESTURE_THRESHOLD_ANGLE_VALUE) {

            if (abs(prePitch - pitch) > 10 || abs(preRoll - roll) > 10) {
                LOGD("nosing.. pitch [%d %d]   roll [%d %d]", prePitch, pitch, preRoll, roll);
                gestureOK = 1;
                gestureOK_wait = 0;
                return -1;
            }

            if (preYaw - yaw > 0) {
                ret = 1;
                LOGD("capture moving right %d %d", preYaw, yaw);
            }
            else {
                ret = 2;
                LOGD("capture moving left %d %d", preYaw, yaw);
            }

            gestureOK = 1;
            gestureOK_wait = 0;
            return ret;
        }


        if (abs(prePitch - pitch) > GESTURE_THRESHOLD_ANGLE_VALUE) {

            if (abs(preYaw - yaw) > GESTURE_THRESHOLD_ANGLE_VALUE || abs(preRoll - roll) > GESTURE_THRESHOLD_ANGLE_VALUE) {
                LOGD("nosing..");
                gestureOK = 1;
                gestureOK_wait = 0;
                return -1;
            }

            if (prePitch - pitch > 0) {
                LOGD("capture moving up %d %d", prePitch, pitch);
            }
            else {
                LOGD("capture moving down %d %d", prePitch, pitch);
            }

            gestureOK = 1;
            gestureOK_wait = 0;
            return -1;
        }


        if (sample->time_us - startAction > GESTURE_DOING_TIME) {
            startAction = 0;

            preYaw = 0;
            prePitch = 0;
            preRoll = 0;

            return -1;
        }

        return -1;
    }

    if (abs(preYaw - yaw) > 1 || abs(prePitch - pitch) > 1) {

        preYaw = yaw;
        prePitch = pitch;
        preRoll = roll;

        startAction = sample->time_us;
    }

    return -1;
}

// 在移动的阀值
#define GESTURE_RAW_GRYOZ_THRESHOLD_VALUE   100

int process_gesture_raw(struct imu_sample *sample, int pitch, int roll, int yaw)
{
    static int gyrozOffset = 0;
    static uint64_t startTicket = 0;

    if (gestureOK == 1) {

        if (abs(sample->data[3]) > 200 || \
            abs(sample->data[4]) > 200 || \
            abs(sample->data[5]) > 200) {

            gestureOK_wait = 0;

            //LOGD("%05d %05d %05d", sample->data[3], sample->data[4], sample->data[5]);
            return -1;
        } else {
            gestureOK_wait++;

            // static 状态计数 > 10，每一帧是20ms，即200ms左右。

            if (gestureOK_wait > 5) {
                gestureOK = 0;
                gestureOK_wait = 0;

                return -1;
            }

            return -1;
        }
    }

    if (abs(sample->data[5]) > GESTURE_RAW_GRYOZ_THRESHOLD_VALUE) {
        gyrozOffset += abs(sample->data[5]);
        startTicket = sample->time_us;
    }
    else {
        if (sample->time_us - startTicket > 100*1000) {
            gyrozOffset = 0;
        }
    }

    if (gyrozOffset > 6000) {
        LOGD("gyrozOffset = %d", gyrozOffset);
        gestureOK = 1;
        gestureOK_wait = 0;
    }

    return -1;
}


int procee_gesture_clicked(struct imu_sample *sample, int pitch, int roll, int yaw)
{
    int ret = 0 ;

    static int startAcc[5] = {0};
    static int acc_count = 0;
    static int acc_start_flag = 0;
    static int acc_total = 0;

    static short preAccx = -1;
    static short preAccy = -1;
    static short preAccz = -1;
    static short preGyrx = -1;
    static short preGyry = -1;
    static short preGyrz = -1;

    if (preAccx == -1) {
        preAccx = sample->data[0];
        preAccy = sample->data[1];
        preAccz = sample->data[2];

        preGyrx = sample->data[3];
        preGyry = sample->data[4];
        preGyrz = sample->data[5];

        return -1;
    }

    if (0/*gestureOK == 1*/) {
        acc_count = 0;
        acc_start_flag = 0;
        acc_start_flag = 0;
        return -1;

        static int gestureOK_timeus = -1;

        if (gestureOK_timeus == -1) {
            gestureOK_timeus = sample->time_us;
        }

        if (sample->time_us - gestureOK_timeus < 200*1000) {
            return -1;
        }
    }

    int accn = abs(sample->data[0] - preAccx) + abs(sample->data[1] - preAccy) + abs(sample->data[2] - preAccz);
    int gyrn = abs(sample->data[3] - preGyrx) + abs(sample->data[4] - preGyry) + abs(sample->data[5] - preGyrz);

    preAccx = sample->data[0];
    preAccy = sample->data[1];
    preAccz = sample->data[2];

    preGyrx = sample->data[3];
    preGyry = sample->data[4];
    preGyrz = sample->data[5];

    //if(accn < 500 && gyrn < 500) {
    //    return -1;
    //}

    //LOGD("%06d %06d %08d", accn, gyrn, sample->time_us/1000);



    if(accn > 1000) {
        if (acc_count >= 5) {
            if (acc_start_flag == 0) {
                LOGD("acc start ..");
            }
            acc_start_flag = 1;
        }
        else {
            startAcc[acc_count++] = accn;
        }

        if (acc_start_flag == 1) {
            acc_total += accn;
        }
    }
    else {
        acc_count = 0;
        if (acc_start_flag == 1) {

            acc_total += startAcc[0];
            acc_total += startAcc[1];
            acc_total += startAcc[2];
            acc_total += startAcc[3];
            acc_total += startAcc[4];

            LOGD("acc end ..%06d", acc_total);

            if (acc_total > 20000) {
                ret = 8;
                LOGD("clicked!   %06d %08d", acc_total, sample->time_us / 1000);
            }
        }

        acc_start_flag = 0;
        acc_total = 0;
        return ret;
    }

    /*
    LOGD("%05d %05d %05d   %05d %05d %05d", \
            sample->data[0], sample->data[1], sample->data[2], \
            sample->data[3], sample->data[4], sample->data[5]
    );
    */
    return -1;
}

int procee_gesture_lr(struct imu_sample *sample, int pitch, int roll, int yaw)
{
    static short preAccx = -1;
    static short preAccy = -1;
    static short preAccz = -1;
    static short preGyrx = -1;
    static short preGyry = -1;
    static short preGyrz = -1;

    if (preAccx == -1) {
        preAccx = sample->data[0];
        preAccy = sample->data[1];
        preAccz = sample->data[2];

        preGyrx = sample->data[3];
        preGyry = sample->data[4];
        preGyrz = sample->data[5];

        return -1;
    }

    int gyrn = abs(sample->data[5] - preGyrz);

    preAccx = sample->data[0];
    preAccy = sample->data[1];
    preAccz = sample->data[2];

    preGyrx = sample->data[3];
    preGyry = sample->data[4];
    preGyrz = sample->data[5];


    LOGD("gyrn ..%06d", gyrn);
    return -1;

    static int startGyr[5] = {0};
    static int gyr_count = 0;
    static int gyr_start_flag = 0;
    static int gyr_total = 0;

    if (gyrn > 60) {
        if (gyr_count >= 3) {
            if (gyr_start_flag == 0) {
                LOGD("gyr start ..");
            }
            gyr_start_flag = 1;
        }
        else {
            startGyr[gyr_count++] = gyrn;
        }

        if (gyr_start_flag == 1) {
            gyr_total += gyrn;
        }
    }
    else {
        gyr_count = 0;
        if (gyr_start_flag == 1) {

            gyr_total += startGyr[0];
            gyr_total += startGyr[1];
            gyr_total += startGyr[2];
            //gyr_total += startGyr[3];
            //gyr_total += startGyr[4];

            LOGD("gyr end ..%06d", gyr_total);

            if (gyr_total > 10000) {
                LOGD("%06d %08d", gyr_total, sample->time_us / 1000);
            }
        }

        gyr_start_flag = 0;
        gyr_total = 0;
    }

    /*
    LOGD("%05d %05d %05d   %05d %05d %05d", \
            sample->data[0], sample->data[1], sample->data[2], \
            sample->data[3], sample->data[4], sample->data[5]
    );
    */
    return -1;
}
