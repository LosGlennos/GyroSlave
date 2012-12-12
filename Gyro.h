#ifndef GYRO_H
#define	GYRO_H

void delayms(int count);
void DataTx(char data);
char DataRx(void);
void InitializeGyro(void);
void ReadAngularRate(short *angularrate);
void SumAngular(int *angularrate, int *angularSum);
void ResetAngularSum(int *angularSum);

#endif	/* GYRO_H */

