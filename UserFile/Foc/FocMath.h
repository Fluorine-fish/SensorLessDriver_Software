/**
*   @file FocMath.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/29
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_FOCMATH_H
#define VESC_F4_FOCMATH_H

#define PI 3.1415926f
#define TWO_THREES 0.6666666667f
#define TWO_THREES_PI 2.09439510239f
#define FOUR_THREES_PI 4.18879020489f
#define HALF_SQRT3 0.8660254038f
#define INV_2PI 0.1591549431f
#define HOLLYST 0.017453292519943295769236907684886f

float FocMath_FastSin(float angle);
float FocMath_FastCos(float angle);

#endif //VESC_F4_FOCMATH_H
