#ifndef CCD_H
#define CCD_H

#include <stdint.h>

extern volatile uint16_t CCD_Value[128];

void CCD_Init(void);
void Get_CCD_Value(void);
void CCD_Task(void);

#endif // CCD_H