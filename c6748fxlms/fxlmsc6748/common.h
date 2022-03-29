/*
 * common.h
 *
 *  Created on: 2022. 3. 29.
 *      Author: jyheo
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <math.h>

#define PI (3.14159265359f)
#define PI2 (6.28318530718f)

#define SHORT_TO_FLOAT(x) ((float)(x)/(float)32768.f)
#define FLOAT_TO_SHORT(x) ((short)(x)*32767);


#define SHORT_TO_FLOAT_ARRAY(x, size) \
        for(i = 0; i < size; i++) \
        {\
            x[i] = SHORT_TO_FLOAT(x[i]);\
        }

static struct ReferenceSignal{
    float qn0;
    float qn1;
    float qn2;
    float cos_f0;
    float sin_f0;
    float cos_f1;
    float sin_f1;
    float cos_f2;
    float sin_f2;
};

#endif /* COMMON_H_ */
