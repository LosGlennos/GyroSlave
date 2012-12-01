/* 
 * File:   Gyro.h
 * Author: marti_000
 *
 * Created on den 28 november 2012, 13:57
 */

#ifndef GYRO_H
#define	GYRO_H

void delayms(int count);
void DataTx(char data);
char DataRx();
void InitializeGyro();
void ReadAngularRate(int *angularrate);
void SumAngular(int *angularrate, int *angularSum);
void ResetAngularSum(int *angularSum);

#endif	/* GYRO_H */

