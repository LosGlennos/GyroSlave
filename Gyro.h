#ifndef GYRO_H
#define	GYRO_H

void delayms(int count);
void DataTx(char data);
char DataRx(void);
void InitializeGyro(void);
//short ReadAngularRate();
void ReadAngularRate(short* angular);
void SumAngular(int *angular, int *Sum);
void ResetAngularSum(int *angularSum);

#endif	/* GYRO_H */

