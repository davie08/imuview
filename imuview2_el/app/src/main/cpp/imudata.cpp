//
// Created by Liio Chen on 2022/1/4.
//
#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include "RingBuffer.h"
#include "imudata.h"

extern void IMUSO3Thread(struct imu_sample *imu_data, int* pitch, int* roll, int *yaw);
extern void HWS_InitIMU(void);
extern int ReadIMUSensorHandle(struct imu_sample *imu_data);

extern int process_gesture(struct imu_sample *sample, int pitch, int roll, int yaw);
extern int process_gesture_raw(struct imu_sample *sample, int pitch, int roll, int yaw);
extern int procee_gesture_clicked(struct imu_sample *sample, int pitch, int roll, int yaw);
extern int procee_gesture_lr(struct imu_sample *sample, int pitch, int roll, int yaw);

#define LOG_TAG "IMUDATA_NATIVE"

#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

RingBuffer<imu_sample> *imu_buffer = nullptr;
struct imu_sample calibration_data = { 0 };
static bool calibration_on = false;

extern "C" JNIEXPORT void JNICALL
Java_com_toynext_imuview_BluetoothLeService_IMUInitData(
        JNIEnv* env,
        jobject /* this */
)
{
    HWS_InitIMU();

    LOGD("HWS_InitIMU");

    imu_buffer = new RingBuffer<imu_sample>(IMU_DEFAULT_HZ);
    if (imu_buffer == nullptr) {
        return ;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_toynext_imuview_BluetoothLeService_IMUFreeData(
        JNIEnv* env,
        jobject /* this */
)
{
    if (imu_buffer) {
        delete imu_buffer;
        imu_buffer = nullptr;
    }
}

extern "C" JNIEXPORT int JNICALL
Java_com_toynext_imuview_BluetoothLeService_parseData(
        JNIEnv* env,
        jobject /* this */,
        jbyteArray data) {

    imu_sample imu_data;
    jsize size = env->GetArrayLength(data);
    if (size < 12) {
        return -1;
    }

    if (calibration_on) {
        return -1;
    }

    unsigned char *value = (unsigned char *) env->GetByteArrayElements(data, nullptr);

    imu_data.data[0] = (short)value[1] << 8 | value[0];
    imu_data.data[1] = (short)value[3] << 8 | value[2];
    imu_data.data[2] = (short)value[5] << 8 | value[4];


    imu_data.data[3] = (short)value[6] << 8 | value[7];
    imu_data.data[4] = (short)value[8] << 8 | value[9];
    imu_data.data[5] = (short)value[10] << 8 | value[11];

    if (calibration_data.time_us) {
        imu_data.data[0] -= calibration_data.data[0];
        imu_data.data[1] -= calibration_data.data[1];
        imu_data.data[2] -= calibration_data.data[2];
        imu_data.data[3] -= calibration_data.data[3];
        imu_data.data[4] -= calibration_data.data[4];
        imu_data.data[5] -= calibration_data.data[5];
    }

    /*
    LOGD("%llu: \tAccel X: %7.02f, Y: %7.02f, Z: %7.02f\t Gyro X: %7.02f, Y: %7.02f, Z: %7.02f",
         imu_data.time_us, imu_data.data[0] * 0.488f, imu_data.data[1] * 0.488f, imu_data.data[2] * 0.488f,
         imu_data.data[3] * 0.07f, imu_data.data[4] * 0.07f, imu_data.data[5] * 0.07f);
    */

    //int pitch, roll, yaw;
    //IMUSO3Thread(&imu_data, &pitch, &roll, &yaw);

    imu_data.time_us = nanoseconds_to_microseconds(getSystemTime());

    imu_buffer->push(imu_data);

    return ReadIMUSensorHandle(&imu_data);

    //process_gesture_raw(&imu_data, pitch, roll, yaw);
    //procee_gesture_clicked(&imu_data, pitch, roll, yaw);
    //procee_gesture_lr(&imu_data, pitch, roll, yaw);

#if 0
    int ret = process_gesture(&imu_data, pitch, roll, yaw);

    if (ret != -1) {
        return ret;
    }

    return procee_gesture_clicked(&imu_data, pitch, roll, yaw);
#endif

    return -1;
}

extern "C" JNIEXPORT void JNICALL
Java_com_toynext_imuview_BluetoothLeService_Calibration(
        JNIEnv* env,
        jobject /* this */,
        jint delay_sec)
{
    int samples = 0;
    uint64_t start_time = nanoseconds_to_seconds(getSystemTime());
    int64_t t[6] = {0};
    struct imu_sample data;
    if (imu_buffer == nullptr) {
        return ;
    }

    LOGD("Calibration enter, cleanup pool. total: %d", imu_buffer->get_length());

    calibration_on = true;
    while(imu_buffer->pop_first_older_than(nanoseconds_to_microseconds(getSystemTime()), &data)) {
        samples++;
    }
    calibration_on = false;
    LOGD("Calibration enter, cleanup done. deletes: %d", samples);

    LOGD("Calibration enter, start...");
    samples = 0;
    while((nanoseconds_to_seconds(getSystemTime()) - start_time) < delay_sec) {
        if (imu_buffer->pop_first_older_than(nanoseconds_to_microseconds(getSystemTime()), &data)) {
            t[0] += data.data[0];
            t[1] += data.data[1];
            t[2] += data.data[2];
            t[3] += data.data[3];
            t[4] += data.data[4];
            t[5] += data.data[5];

            samples++;
        }
    }

    if (samples == 0) {
        LOGE("Failed Calibration IMU, no data ready!");
        return ;
    }

    calibration_data.data[0] = (t[0] / samples);
    calibration_data.data[1] = (t[1] / samples);
    calibration_data.data[2] = (t[2] / samples) - (short) (981 / 0.488f);
    calibration_data.data[3] = (t[3] / samples);
    calibration_data.data[4] = (t[4] / samples);
    calibration_data.data[5] = (t[5] / samples);
    calibration_data.time_us = nanoseconds_to_microseconds(getSystemTime());
    LOGD("Calibration enter, done, samples: %d", samples);

    LOGE("%llu: Calibration: \tAccel X: %d, Y: %d, Z: %d\t Gyro X: %d, Y: %d, Z: %d",
         calibration_data.time_us, calibration_data.data[0], calibration_data.data[1], calibration_data.data[2],
         calibration_data.data[3], calibration_data.data[4], calibration_data.data[5]);
}

extern "C" JNIEXPORT jintArray JNICALL
Java_com_toynext_imuview_BluetoothLeService_ReadIMU(
        JNIEnv* env,
        jobject /* this */)
{
    struct imu_sample data;
    if (imu_buffer == nullptr)
        return nullptr;

    if (!imu_buffer->pop_first_older_than(nanoseconds_to_microseconds(getSystemTime()), &data)) {
        return nullptr;
    }

    jintArray raw = env->NewIntArray(IMU_ARRAY_SIZE);
    jint values[IMU_ARRAY_SIZE];

    values[0] = data.data[0];
    values[1] = data.data[1];
    values[2] = data.data[2];
    values[3] = data.data[3];
    values[4] = data.data[4];
    values[5] = data.data[5];

    env->SetIntArrayRegion(raw, (jsize)0, (jsize)IMU_ARRAY_SIZE, values);
    return raw;
}