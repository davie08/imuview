#include "imu.h"
#include "filter.h"
#include <android/log.h>

#define LOG_TAG "IMUDATA_NATIVE"

#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

imu_t imu;
uint8_t imuCaliFlag=0;

//函数名：IMU_Init(void)
//描述：姿态解算融合初始化函数
//现在使用软件解算，不再使用MPU6050的硬件解算单元DMP，IMU_SW在SysConfig.h中定义
void HWS_InitIMU(void)
{
#ifdef IMU_SW		//软解需要先校陀螺
    imu.ready=0;
#else
    imu.ready=1;
#endif
    imu.caliPass=1;
    //filter rate
    LPF2pSetCutoffFreq_1(IMU_SAMPLE_RATE, IMU_FILTER_CUTOFF_FREQ);		//30Hz
    LPF2pSetCutoffFreq_2(IMU_SAMPLE_RATE, IMU_FILTER_CUTOFF_FREQ);
    LPF2pSetCutoffFreq_3(IMU_SAMPLE_RATE, IMU_FILTER_CUTOFF_FREQ);
    LPF2pSetCutoffFreq_4(IMU_SAMPLE_RATE, IMU_FILTER_CUTOFF_FREQ);
    LPF2pSetCutoffFreq_5(IMU_SAMPLE_RATE, IMU_FILTER_CUTOFF_FREQ);
    LPF2pSetCutoffFreq_6(IMU_SAMPLE_RATE, IMU_FILTER_CUTOFF_FREQ);
}

#define SENSOR_MAX_G 16.0f		//constant g		// tobe fixed to 8g. but IMU need to  correct at the same time
#define SENSOR_MAX_W 2000.0f	//deg/s
#define ACC_SCALE  (SENSOR_MAX_G/32768.0f)
#define GYRO_SCALE  (SENSOR_MAX_W/32768.0f)

int gesture_x(float accx, float accy, float accz, float gyrox, float gyroy, float gyroz);

int ReadIMUSensorHandle(struct imu_sample *imu_data)
{
    uint8_t i;

    imu.accADC[0] = imu_data->data[0];
    imu.accADC[1] = imu_data->data[1];
    imu.accADC[2] = imu_data->data[2];
    imu.gyroADC[0] = imu_data->data[3];
    imu.gyroADC[1] = imu_data->data[4];
    imu.gyroADC[2] = imu_data->data[5];

    //tutn to physical
    for(i=0; i<3; i++)
    {
        imu.accRaw[i]= (float)imu.accADC[i] *ACC_SCALE * CONSTANTS_ONE_G ;
        imu.gyroRaw[i]=(float)imu.gyroADC[i] * GYRO_SCALE * M_PI_F /180.f;		//deg/s
    }
#if 1
    imu.accb[0] = LPF2pApply_1(imu.accRaw[0]-imu.accOffset[0]);
    imu.accb[1] = LPF2pApply_2(imu.accRaw[1]-imu.accOffset[1]);
    imu.accb[2] = LPF2pApply_3(imu.accRaw[2]-imu.accOffset[2]);

#ifdef IMU_SW
    imu.gyro[0] = LPF2pApply_4(imu.gyroRaw[0]);
    imu.gyro[1] = LPF2pApply_5(imu.gyroRaw[1]);
    imu.gyro[2] = LPF2pApply_6(imu.gyroRaw[2]);
#else
    imu.gyro[0]=LPF2pApply_4(imu.gyroRaw[0]-imu.gyroOffset[0]);
    imu.gyro[1]=LPF2pApply_5(imu.gyroRaw[1]-imu.gyroOffset[1]);
    imu.gyro[2]=LPF2pApply_6(imu.gyroRaw[2]-imu.gyroOffset[2]);
#endif
#else

    imu.accb[0] = (imu.accRaw[0]-imu.accOffset[0]);
    imu.accb[1] = (imu.accRaw[1]-imu.accOffset[1]);
    imu.accb[2] = (imu.accRaw[2]-imu.accOffset[2]);

    imu.gyro[0] = (imu.gyroRaw[0]);
    imu.gyro[1] = (imu.gyroRaw[1]);
    imu.gyro[2] = (imu.gyroRaw[2]);
#endif

    return gesture_x(imu.accb[0], imu.accb[1], imu.accb[2], imu.gyro[0], imu.gyro[1], imu.gyro[2]);

    //LOGD(" %7.02f  %7.02f  %7.02f    %7.02f  %7.02f  %7.02f", \
    //                        imu.accb[0], imu.accb[1], imu.accb[2], \
    //                        imu.gyro[0], imu.gyro[1], imu.gyro[2]);
}

struct saved_imu_sample {
    float data[6];
};

struct saved_imu_sample saved_sample[1000];

static int acc_count = 0;
static int acc_start_flag = 0;
static float startAcc[1000];
static float acc_total = 0;

static int gyro_count = 0;
static int gyro_start_flag = 0;
static float startGyro[1000];
static float gyro_total = 0;

#define EPISON 1e-7
#define MOVE_LEFT       2
#define MOVE_RIGHT      1
#define MOVE_CLICKED    8
#define MOVE_UP         3
#define MOVE_DOWN       4

int is_single_gyroz(int E, int L)
{
    int a1 = 1;
    int a2 = 1;
    int flag = 0;

    int M = L/2;
    int aM = (saved_sample[M].data[5] >= EPISON)?1:0;

    float positive_z = 0.0;    //+
    float negative_z = 0.0;    //-

    int positive_z_count = 0;    //+
    int negative_z_count = 0;    //-

    float positive_x = 0.0;    //+
    float negative_x = 0.0;    //-

    float positive_y = 0.0;    //+
    float negative_y = 0.0;    //-

    for (int i=0; i < L; i++) {
        if (saved_sample[i].data[3] >= EPISON) {
            positive_x += saved_sample[i].data[3];
        }
        else {
            negative_x += saved_sample[i].data[3];
        }

        if (saved_sample[i].data[4] >= EPISON) {
            positive_y += saved_sample[i].data[4];
        }
        else {
            negative_y += saved_sample[i].data[4];
        }

        if (saved_sample[i].data[5] >= EPISON) {
            positive_z += saved_sample[i].data[5];
            positive_z_count++;
        }
        else {
            negative_z += saved_sample[i].data[5];
            negative_z_count++;
        }
    }

    LOGD("%7.02f %7.02f ", positive_x, negative_x);
    LOGD("%7.02f %7.02f ", positive_y, negative_y);
    LOGD("%7.02f %7.02f ", positive_z, negative_z);

    if ((abs(positive_y) + abs(negative_y)) > (abs(positive_x) + abs(negative_x)) &&
        (abs(positive_y) + abs(negative_y)) > ((abs(positive_z) + abs(negative_z))*2)
        ) {
        // y轴角速度特征，左右侧翻
        return -1;
    }

    if ((E / L) > 10 && E > 300 && (abs(positive_x) + abs(negative_x)) > ((abs(positive_z) + abs(negative_z))*2) ) {
        LOGD("clicked!!!");
        return MOVE_CLICKED;
    }

    if ((abs(positive_x) + abs(negative_x)) > (abs(positive_z) + abs(negative_z))*2) {
        if (abs(negative_x) > abs(negative_x)) {
            return MOVE_UP;
        }
        else {
            return MOVE_DOWN;
        }
    }
    else {

        //if (abs(abs(positive_z_count) - abs(negative_z_count)) <= 2) {
        //    return -1;  // z轴正负值数量相当？
        //}

        if (abs(positive_z) + abs(negative_z) < 10) {
            return -1;  // z轴旋转能量太小
        }

        if (abs(positive_z) > abs(negative_z)) {
            return MOVE_LEFT;
        } else {
            return MOVE_RIGHT;
        }
    }



    for (int i=M; i >= 0; i--) {
        int ai = saved_sample[i].data[5] >= EPISON;

        if (ai != aM && (saved_sample[i].data[5] > 0.05)) {
            for (; i >=0; i--) {
                int ai = saved_sample[i].data[5] >= EPISON;

                if (ai == aM) {
                    if (i != 0) {
                        return 0;
                    }
                }
            }

            flag = 1;
        }
    }

    for (int i=M; i < L; i++) {
        int ai = saved_sample[i].data[5] >= EPISON;

        if (ai != aM && (saved_sample[i].data[5] > 0.05)) {

            if (flag == 1 && i != L-1)
                return 0;

            for (; i < L; i++) {
                int ai = saved_sample[i].data[5] >= EPISON;

                if (ai == aM) {
                    if (i != L-1)
                        return 0;
                }
            }
        }
    }

    if (aM == 1) {
        return MOVE_LEFT;
    }

    return MOVE_RIGHT;
}

int process_move(void)
{
    int single_gyroz = 0;
    int ret = -1;

    if (gyro_start_flag == 1 && acc_start_flag == 1) {
        int L = acc_count > gyro_count?gyro_count:acc_count;
        float E = 0.0;

        float Ea = 0.0;
        float Eg = 0.0;

        for (int i=0; i < L; i++) {
            E += abs(saved_sample[i].data[0]);
            E += abs(saved_sample[i].data[1]);
            E += abs(saved_sample[i].data[2] - 9.8);

            E += abs(saved_sample[i].data[3]);
            E += abs(saved_sample[i].data[4]);
            E += abs(saved_sample[i].data[5]);

            Ea += abs(saved_sample[i].data[0]);
            Ea += abs(saved_sample[i].data[1]);
            Ea += abs(saved_sample[i].data[2] - 9.8);

            Eg += abs(saved_sample[i].data[3]);
            Eg += abs(saved_sample[i].data[4]);
            Eg += abs(saved_sample[i].data[5]);
        }

        LOGD("L = %02d  E = %7.02f", L, E);

        if (L < 10 || E < 50) { // 小抖动
            return -1;
        }

        if (L > 50) {   // 原地转动 50次为50*20=1000ms
            return -1;
        }

        single_gyroz = is_single_gyroz(E, L);

        if (single_gyroz == MOVE_CLICKED) {
            LOGD("Ea = %7.02f  Eg = %7.02f", Ea, Eg);
            return MOVE_CLICKED;
        }

        if (single_gyroz == MOVE_LEFT) {
            LOGD("single gyroz left");
            ret = MOVE_LEFT;
        }
        else if (single_gyroz == MOVE_RIGHT) {
            ret = MOVE_RIGHT;
            LOGD("single gyroz right");
        }
        else if (single_gyroz == MOVE_UP) {
            ret = MOVE_UP;
            LOGD("single up");
        }
        else if (single_gyroz == MOVE_DOWN) {
            ret = MOVE_DOWN;
            LOGD("single gyro down");
        }

        for (int i=0; i < L; i++) {
            LOGD("%7.02f %7.02f %7.02f", saved_sample[i].data[3], saved_sample[i].data[4], saved_sample[i].data[5]);
        }

        if (ret == -1 && single_gyroz == 0 && (acc_total / acc_count > 4)) {
            LOGD("clicked!!!");
            ret = MOVE_CLICKED;
        }
    }

    return ret;
}

int gesture_x(float accx, float accy, float accz, float gyrox, float gyroy, float gyroz)
{
    static float preAccx ;
    static float preAccy ;
    static float preAccz ;
    static float preGyrx ;
    static float preGyry ;
    static float preGyrz ;

    static int gesture_start = 0;

    int ret = -1;

    if (gesture_start == 0) {

        preAccx = accx;
        preAccy = accy;
        preAccz = accz;

        preGyrx = gyrox;
        preGyry = gyroy;
        preGyrz = gyroz;

        gesture_start = 1;

        return -1;
    }

    float accn = abs(accx - preAccx) + abs(accy - preAccy) + abs(accz - preAccz);
    float gyrn = abs(gyrox - preGyrx) + abs(gyroy - preGyry) + abs(gyroz - preGyrz);

    preAccx = accx;
    preAccy = accy;
    preAccz = accz;

    preGyrx = gyrox;
    preGyry = gyroy;
    preGyrz = gyroz;

    //LOGD(" %7.02f  %7.02f", accn, gyrn);

    if(accn > 0.3) {
        if (acc_count == 5) {
            LOGD("acc start ..");
            acc_start_flag = 1;
        }

        startAcc[acc_count] = accn;
        saved_sample[acc_count].data[0] = accx;
        saved_sample[acc_count].data[1] = accy;
        saved_sample[acc_count].data[2] = accz;

        acc_count++;
        acc_total += accn;
    }
    else {
        if (acc_start_flag == 1) {
            LOGD("acc end %03d   %7.02f", acc_count, acc_total);

            ret = process_move();
        }

        acc_count = 0;
        acc_start_flag = 0;
        acc_total = 0;
    }


    if(gyrn > 0.2) {
        if (gyro_count == 5) {
            LOGD("gyro start ..");
            gyro_start_flag = 1;
        }

        startGyro[gyro_count] = gyrn;
        saved_sample[gyro_count].data[3] = gyrox;
        saved_sample[gyro_count].data[4] = gyroy;
        saved_sample[gyro_count].data[5] = gyroz;

        gyro_count++;
        gyro_total += gyrn;
    }
    else {
        if (gyro_start_flag == 1) {
            LOGD("gyro end %03d   %7.02f", gyro_count, gyro_total);
            if (ret == -1) {
                ret = process_move();
            }
        }

        gyro_count = 0;
        gyro_start_flag = 0;
        gyro_total = 0;
    }

    return ret;
}