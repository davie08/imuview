#ifndef _IMU_H_
#define _IMU_H_

#include <math.h>

struct imu_sample {
    uint64_t time_us;
    short data[6];
};

#define YAW_CORRECT
#define IMU_SW		//姿态解算使用软件解算，不再使用MPU6050的硬件解算单元DMP
#define HIGH_FREQ_CTRL
#define NEW_RC



#ifdef HIGH_FREQ_CTRL
#define IMU_SAMPLE_RATE 	50.0f
#else
#define IMU_SAMPLE_RATE 			100.0f	//1000.0f/(float)DMP_CALC_PRD
#endif

#define IMU_FILTER_CUTOFF_FREQ	10.0f

//校准时间
#define ACC_CALC_TIME  3000//ms
#define GYRO_CALC_TIME   10000000l	//us

typedef float  quad[4];
typedef float  vector3f[3];	//不可作为返回值，指针
typedef float  matrix3f[3][3];


typedef struct mat3_tt
{
    float m[3][3];
} mat3;

typedef struct vec3_tt
{
    float v[3];
} vec3;


enum {ROLL,PITCH,YAW,THROTTLE};
enum {X,Y,Z};

typedef struct IMU_tt
{
    uint8_t caliPass;
    uint8_t ready;
    int16_t accADC[3];
    int16_t gyroADC[3];
    int16_t magADC[3];
    float 	accRaw[3];		//m/s^2
    float 	gyroRaw[3];		//rad/s
    float 	magRaw[3];		//
    float   accOffset[3];		//m/s^2
    float   gyroOffset[3];
    float   accb[3];		//filted, in body frame
    float   accg[3];
    float   gyro[3];
    float   DCMgb[3][3];
    float   q[4];
    float   roll;				//deg
    float   pitch;
    float 	yaw;
    float   rollRad;				//rad
    float   pitchRad;
    float 	yawRad;
} imu_t;

//enum{ROLL,PITCH,YAW};
#define M_PI_F 3.1415926
#define CONSTANTS_ONE_G					9.80665f		/* m/s^2		*/
#define CONSTANTS_AIR_DENSITY_SEA_LEVEL_15C		1.225f			/* kg/m^3		*/
#define CONSTANTS_AIR_GAS_CONST				287.1f 			/* J/(kg * K)		*/
#define CONSTANTS_ABSOLUTE_NULL_CELSIUS			-273.15f		/* 癈			*/
#define CONSTANTS_RADIUS_OF_EARTH			6371000			/* meters (m)		*/

extern volatile float accFilted[3],gyroFilted[3];
extern float DCMbg[3][3],DCMgb[3][3];
extern float accZoffsetTemp;
extern float IMU_Pitch,IMU_Roll,IMU_Yaw;
extern imu_t imu;
extern uint8_t imuCaliFlag;


void IMU_Init(void);
void IMU_Process(void);
uint8_t IMU_Calibrate(void);
int ReadIMUSensorHandle(struct imu_sample *imu_data);
uint8_t IMUCheck(void);

#endif

