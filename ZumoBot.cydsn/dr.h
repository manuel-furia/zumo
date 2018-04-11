/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#ifndef DR_H
#define DR_H
# define treshold 2300
#include "Reflectance.h"
typedef struct {
   struct sensors_ s;
   float time, lastproportional, derivative;
}sensorData;
void startSensor(sensorData*);
float scale(float , float, float);
void drive(sensorData* d, float *min,float *max, float kp, float kd,float speedSccale);
#endif

/* [] END OF FILE */