#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "cvi_type.h"
#include "ae_debug.h"
#include "aealgo.h"
#include "ae_iris.h"
#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"
#include "cvi_isp.h"
#include "isp_debug.h"
#include "ae_buf.h"

//open it after check iris implement check OK
#define ENABLE_IRIS_CONTROL
#define MAX_PWM_PATH_LENGTH   128
#define DC_IRIS_PWM_PERIOD	   "1000000"
#define DC_IRIS_PWM_DUTY	   "1000000"
#define DC_IRIS_PWM_DEFAULT_PATH	"/sys/class/pwm/pwmchip0"

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation" /* Or  "-Wformat-overflow"  */
#endif

static char aeDCIrisPwmPath[MAX_PWM_PATH_LENGTH] = {0};
static CVI_BOOL bHasDcIris[AE_SENSOR_NUM];
static CVI_BOOL bUseDcIris[AE_SENSOR_NUM];

void AE_IrisAttr_Init(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	bUseDcIris[sID] = bHasDcIris[sID] = 0;
	if (!pstAeIrisAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeIrisAttr is NULL.");
		return;
	}
	memset(pstAeIrisAttr[sID], 0, sizeof(ISP_IRIS_ATTR_S));

	if (!pstAeDcIrisAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeDcIrisAttr is NULL.");
		return;
	}
	memset(pstAeDcIrisAttr[sID], 0, sizeof(ISP_DCIRIS_ATTR_S));
}

void AE_UpdateIrisAttr(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	*pstAeIrisAttrInfo[sID] = *pstAeIrisAttr[sID];
}

void AE_UpdateDcIrisAttr(VI_PIPE ViPipe)
{
	CVI_U8 sID;
	static CVI_U8 irisEnable[AE_SENSOR_NUM];

	sID = AE_ViPipe2sID(ViPipe);

	*pstAeDcIrisAttrInfo[sID] = *pstAeDcIrisAttr[sID];
	if (AE_IsDCIris(sID)) {
		if (!irisEnable[sID]) {
			AE_DCIrisPwm_Init(ViPipe);
			irisEnable[sID] = 1;
		}
	}
}


CVI_S32 AE_SetIrisAttr(CVI_U8 sID, const ISP_IRIS_ATTR_S *pstIrisAttr)
{
	if (!pstAeIrisAttr[sID]) {
		ISP_LOG_ERR("%s\n", "ISP_IRIS_ATTR_S is NULL.");
		return CVI_FAILURE;
	}
	*pstAeIrisAttr[sID] = *pstIrisAttr;
	AE_SetParameterUpdate(sID, AE_IRIS_ATTR_UPDATE);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetIrisAttr(CVI_U8 sID, ISP_IRIS_ATTR_S *pstIrisAttr)
{
	if (!pstAeIrisAttr[sID]) {
		ISP_LOG_ERR("%s\n", "ISP_IRIS_ATTR_S is NULL.");
		return CVI_FAILURE;
	}
	*pstIrisAttr = *pstAeIrisAttr[sID];
	return CVI_SUCCESS;
}

CVI_S32 AE_SetDcIrisAttr(CVI_U8 sID, const ISP_DCIRIS_ATTR_S *pstDcirisAttr)
{
	if (!pstAeDcIrisAttr[sID]) {
		ISP_LOG_ERR("%s\n", "ISP_DCIRIS_ATTR_S is NULL.");
		return CVI_FAILURE;
	}
	*pstAeDcIrisAttr[sID] = *pstDcirisAttr;
	AE_SetParameterUpdate(sID, AE_DC_IRIS_ATTR_UPDATE);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetDcIrisAttr(CVI_U8 sID, ISP_DCIRIS_ATTR_S *pstDcirisAttr)
{
	if (!pstAeDcIrisAttr[sID]) {
		ISP_LOG_ERR("%s\n", "ISP_DCIRIS_ATTR_S is NULL.");
		return CVI_FAILURE;
	}
	*pstDcirisAttr = *pstAeDcIrisAttr[sID];
	return CVI_SUCCESS;
}



CVI_S32 AE_SetDCIrisPwmCtrlPath(const char *szPath)
{
	if (szPath == NULL) {
		printf("error %s = NULL\n", __func__);
		return CVI_FAILURE;
	}
	if (strlen(szPath) >= MAX_PWM_PATH_LENGTH) {
		printf("Path length over %d\n", MAX_PWM_PATH_LENGTH);
		return CVI_FAILURE;
	}
	snprintf(aeDCIrisPwmPath, MAX_PWM_PATH_LENGTH, "%s", szPath);
	return CVI_SUCCESS;
}

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
CVI_S32 AE_DCIrisPwm_Init(VI_PIPE ViPipe)
{
	int fd, fd_period, fd_duty, fd_enable, ret;
	ISP_CTRL_PARAM_S stIspCtrlParam;
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);
	char pwmCtrlName[MAX_PWM_PATH_LENGTH], number[8];

	if (strlen(aeDCIrisPwmPath) == 0) {
		snprintf(aeDCIrisPwmPath, MAX_PWM_PATH_LENGTH, "%s", DC_IRIS_PWM_DEFAULT_PATH);
	}
	CVI_ISP_GetCtrlParam(ViPipe, &stIspCtrlParam);
	snprintf(pwmCtrlName, MAX_PWM_PATH_LENGTH, "%s/export", aeDCIrisPwmPath);
	fd = open(pwmCtrlName, O_WRONLY);
	if (fd < 0) {
		printf("open %s error\n", pwmCtrlName);
		return CVI_FAILURE;
	}
	sprintf(number, "%d", stIspCtrlParam.u32PwmNumber);

	ret = write(fd, number, strlen(number));
	if (ret < 0) {
		printf("Export pwm%d error\n", stIspCtrlParam.u32PwmNumber);
		close(fd);
		return CVI_FAILURE;
	}
	close(fd);
	memset(pwmCtrlName, 0, MAX_PWM_PATH_LENGTH);
	snprintf(pwmCtrlName, MAX_PWM_PATH_LENGTH, "%s/pwm%d/period", aeDCIrisPwmPath, stIspCtrlParam.u32PwmNumber);
	fd_period = open(pwmCtrlName, O_RDWR);
	if (fd_period < 0) {
		printf("open period error\n");
		return CVI_FAILURE;
	}
	ret = write(fd_period, DC_IRIS_PWM_PERIOD, strlen(DC_IRIS_PWM_PERIOD));
	if (ret < 0) {
		printf("Set period error\n");
		close(fd_period);
		return CVI_FAILURE;
	}
	close(fd_period);

	memset(pwmCtrlName, 0, MAX_PWM_PATH_LENGTH);
	snprintf(pwmCtrlName, MAX_PWM_PATH_LENGTH, "%s/pwm%d/duty_cycle", aeDCIrisPwmPath, stIspCtrlParam.u32PwmNumber);
	fd_duty = open(pwmCtrlName, O_RDWR);
	if (fd_duty < 0) {
		printf("open %s error\n", pwmCtrlName);
		return CVI_FAILURE;
	}
	ret = write(fd_duty, DC_IRIS_PWM_DUTY, strlen(DC_IRIS_PWM_DUTY));
	if (ret < 0) {
		printf("Set fd_duty error\n");
		close(fd_duty);
		return CVI_FAILURE;
	}
	close(fd_duty);

	memset(pwmCtrlName, 0, MAX_PWM_PATH_LENGTH);
	snprintf(pwmCtrlName, MAX_PWM_PATH_LENGTH, "%s/pwm%d/enable", aeDCIrisPwmPath, stIspCtrlParam.u32PwmNumber);
	fd_enable = open(pwmCtrlName, O_RDWR);
	if (fd_enable < 0) {
		printf("open %s error\n", pwmCtrlName);
		return CVI_FAILURE;
	}
	ret = write(fd_enable, "1", strlen("1"));
	if (ret < 0) {
		printf("enable pwm%d error\n", stIspCtrlParam.u32PwmNumber);
		close(fd_enable);
		return CVI_FAILURE;
	}
	close(fd_enable);
	bHasDcIris[sID] = 1;
	return CVI_SUCCESS;
}
#else
CVI_S32 AE_DCIrisPwm_Init(VI_PIPE ViPipe)
{
	(void)ViPipe;
	return CVI_SUCCESS;
}
#endif

CVI_S32 AE_DCIrisDeltaIntg_PID(CVI_U8 sID, CVI_S32 encoder, CVI_S32 target, CVI_U8 reset)
{
	CVI_FLOAT positionKp, positionKi, positionKd;
	static CVI_U8 minDutyCnt[AE_SENSOR_NUM];
	CVI_S32 s32MinPwmDuty = pstAeDcIrisAttrInfo[sID]->u32MinPwmDuty * 1000;
	CVI_S32 s32MaxPwmDuty = pstAeDcIrisAttrInfo[sID]->u32MaxPwmDuty * 1000;
	CVI_S32 s32HoldValue = pstAeIrisAttrInfo[sID]->stMIAttr.u32HoldValue * 1000;
	CVI_S32 s32InitPwmDuty = s32HoldValue > s32MinPwmDuty ? s32HoldValue : s32MaxPwmDuty / 2;
	static CVI_FLOAT bias[AE_SENSOR_NUM], integralBias[AE_SENSOR_NUM], lastBias[AE_SENSOR_NUM];
	static CVI_S32 s32CurPwmDuty[AE_SENSOR_NUM];
	CVI_FLOAT biasDelta;
	CVI_FLOAT intg_rate = 1;
	CVI_S32 pwm = 0;

	positionKp = pstAeDcIrisAttrInfo[sID]->s32Kp / 100.0;
	positionKi = pstAeDcIrisAttrInfo[sID]->s32Ki / 100.0;
	positionKd = pstAeDcIrisAttrInfo[sID]->s32Kd / 100.0;

	if (s32CurPwmDuty[sID] == 0)
		s32CurPwmDuty[sID] = s32InitPwmDuty;

	if (reset) {
		s32CurPwmDuty[sID] = s32InitPwmDuty;
		bias[sID] = 0;
		lastBias[sID] = 0;
		integralBias[sID] = 0;
	}

	bias[sID] = encoder - target;

	if (s32CurPwmDuty[sID] > s32MaxPwmDuty) { // Anti-integral windup
		if (abs((CVI_S32)bias[sID]) > 200) {
			intg_rate = 0;
		} else {
			intg_rate = 1;
			if (bias[sID] < 0)
				integralBias[sID] += bias[sID];
		}
	} else if (s32CurPwmDuty[sID] < s32MinPwmDuty) {
		if (abs((CVI_S32)bias[sID]) > 200) {
			intg_rate = 0;
		} else {
			intg_rate = 1;
			if (bias[sID] > 0)
				integralBias[sID] += bias[sID];
		}
	} else {
		if (abs((CVI_S32)bias[sID]) > 200) {
			intg_rate = 0;
		} else {
			intg_rate = 1;
			integralBias[sID] += bias[sID];
		}
	}

	biasDelta = bias[sID] - lastBias[sID];
	if (abs((CVI_S32)(bias[sID] - lastBias[sID])) > 100) {
		if (biasDelta > 0)
			biasDelta = 100;
		else
			biasDelta = -100;
	}
	pwm = positionKp * bias[sID] + intg_rate * positionKi * integralBias[sID] + positionKd * biasDelta;

	if (AE_GetDebugMode(sID) == AE_DEBUG_PRINT_IRIS_INFO) {
		printf("Kp=%f KI=%f KD=%f CurPwmDuty=%d lB=%f b=%f\n",
			positionKp * bias[sID], intg_rate * positionKi * integralBias[sID],
			positionKd * biasDelta, s32CurPwmDuty[sID], lastBias[sID], bias[sID]);
	}
	lastBias[sID] = bias[sID];

	s32CurPwmDuty[sID] -= pwm;
	if (s32CurPwmDuty[sID] < s32MinPwmDuty) {
		s32CurPwmDuty[sID] = s32MinPwmDuty;
	} else if (s32CurPwmDuty[sID]  > s32MaxPwmDuty) {
		s32CurPwmDuty[sID] = s32MaxPwmDuty;
	}

	if (s32CurPwmDuty[sID] == s32MinPwmDuty) {
		if (minDutyCnt[sID] > 10)
			integralBias[sID] = 0;
		else
			minDutyCnt[sID]++;
	} else
		minDutyCnt[sID] = 0;
	return s32CurPwmDuty[sID];
}


CVI_S32 AE_DCIrisPosition_PID(CVI_U8 sID, CVI_S32 encoder, CVI_S32 target, CVI_U8 reset)
{
	CVI_FLOAT positionKp, positionKi, positionKd;
	CVI_S32 s32MinPwmDuty = pstAeDcIrisAttrInfo[sID]->u32MinPwmDuty * 1000;
	CVI_S32 s32MaxPwmDuty = pstAeDcIrisAttrInfo[sID]->u32MaxPwmDuty * 1000;
	CVI_S32 s32HoldValue = pstAeIrisAttrInfo[sID]->stMIAttr.u32HoldValue * 1000;
	static CVI_FLOAT bias[AE_SENSOR_NUM], integralBias[AE_SENSOR_NUM], lastBias[AE_SENSOR_NUM];
	static CVI_S32 s32CurPwmDuty[AE_SENSOR_NUM];
	CVI_S32 position_init = s32HoldValue > s32MinPwmDuty ? s32HoldValue : (s32MaxPwmDuty + s32MinPwmDuty) / 2;
	CVI_FLOAT biasDelta;

	positionKp = pstAeDcIrisAttrInfo[sID]->s32Kp / 100.0;
	positionKi = pstAeDcIrisAttrInfo[sID]->s32Ki / 100.0;
	positionKd = pstAeDcIrisAttrInfo[sID]->s32Kd / 100.0;

	if (reset) {
		bias[sID] = 0;
		lastBias[sID] = 0;
		integralBias[sID] = 0;
	}

	bias[sID] = target - encoder;

	if (positionKi * integralBias[sID] > position_init) { // Anti-integral windup
		if (bias[sID] < 0)
			integralBias[sID] += bias[sID];
	} else if (positionKi * integralBias[sID] < -position_init) {
		if (bias[sID] > 0)
			integralBias[sID] += bias[sID];
	} else {
		integralBias[sID] += bias[sID];
	}

	biasDelta = bias[sID] - lastBias[sID];
	if (abs((CVI_S32)(bias[sID] - lastBias[sID])) > 100) {
		if (biasDelta > 0)
			biasDelta = 100;
		else
			biasDelta = -100;
	}

	s32CurPwmDuty[sID] = positionKp * bias[sID] + positionKi * integralBias[sID] + positionKd * biasDelta;
	s32CurPwmDuty[sID] += position_init;
	if (AE_GetDebugMode(sID) == AE_DEBUG_PRINT_IRIS_INFO) {
		printf("Kp=%f KI=%f KD=%f CurPwmDuty=%d pos_init=%d lB=%f b=%f\n",
			positionKp * bias[sID], positionKi * integralBias[sID], positionKd * biasDelta,
			s32CurPwmDuty[sID], position_init, lastBias[sID], bias[sID]);
	}
	lastBias[sID] = bias[sID];

	if (s32CurPwmDuty[sID] < s32MinPwmDuty) {
		s32CurPwmDuty[sID] = s32MinPwmDuty;
	} else if (s32CurPwmDuty[sID]  > s32MaxPwmDuty) {
		s32CurPwmDuty[sID] = s32MaxPwmDuty;
	}

	return s32CurPwmDuty[sID];
}

CVI_S32 AE_DCIrisDelta_PID(CVI_U8 sID, CVI_S32 encoder, CVI_S32 target, CVI_U8 reset)
{
	CVI_FLOAT deltaKp, deltaKi, deltaKd;
	CVI_S32 s32MinPwmDuty = pstAeDcIrisAttrInfo[sID]->u32MinPwmDuty * 1000;
	CVI_S32 s32MaxPwmDuty = pstAeDcIrisAttrInfo[sID]->u32MaxPwmDuty * 1000;
	CVI_S32 s32HoldValue = pstAeIrisAttrInfo[sID]->stMIAttr.u32HoldValue * 1000;
	static CVI_FLOAT bias[AE_SENSOR_NUM], lastBias[AE_SENSOR_NUM], lastlastBias[AE_SENSOR_NUM];
	static CVI_S32 s32CurPwmDuty[AE_SENSOR_NUM];
	CVI_S32 delta_pwm = 0;

	deltaKp = pstAeDcIrisAttrInfo[sID]->s32Kp / 100.0;
	deltaKi = pstAeDcIrisAttrInfo[sID]->s32Ki / 100.0;
	deltaKd = pstAeDcIrisAttrInfo[sID]->s32Kd / 100.0;

	if (s32CurPwmDuty[sID] == 0)
		s32CurPwmDuty[sID] = s32HoldValue > s32MinPwmDuty ? s32HoldValue : s32MaxPwmDuty / 2;

	if (reset) {
		bias[sID] = 0;
		lastBias[sID] = 0;
		lastlastBias[sID] = 0;
		s32CurPwmDuty[sID] = s32HoldValue > s32MinPwmDuty ? s32HoldValue : s32MaxPwmDuty / 2;
	}

	bias[sID] = target - encoder;
	if (abs((CVI_S32)(bias[sID] - lastBias[sID])) > 200) {
		lastBias[sID] = (bias[sID] + lastBias[sID]) / 2;
	}
	delta_pwm = deltaKp * (bias[sID] - lastBias[sID]) + deltaKi * bias[sID]
				+ deltaKd * (bias[sID] - 2 * lastBias[sID] + lastlastBias[sID]);

	s32CurPwmDuty[sID] += delta_pwm;
	if (AE_GetDebugMode(sID) == AE_DEBUG_PRINT_IRIS_INFO) {
		printf("Kp=%f KI=%f KD=%f CurPwmDuty=%d delta=%d lB=%f b=%f\n",
			deltaKp*(bias[sID] - lastBias[sID]), deltaKi*bias[sID],
			deltaKd*(bias[sID] - 2 * lastBias[sID] + lastlastBias[sID]),
			s32CurPwmDuty[sID], delta_pwm, lastBias[sID], bias[sID]);
	}
	lastlastBias[sID] = lastBias[sID];
	lastBias[sID] = bias[sID];

	if (s32CurPwmDuty[sID] < s32MinPwmDuty) {
		s32CurPwmDuty[sID] = s32MinPwmDuty;
	} else if (s32CurPwmDuty[sID]  > s32MaxPwmDuty) {
		s32CurPwmDuty[sID] = s32MaxPwmDuty;
	}

	return s32CurPwmDuty[sID];
}

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
CVI_S32 AE_DCIrisPwmCtrl(VI_PIPE ViPipe, CVI_S32 pwm)
{
	CVI_S32 fd_duty, ret;
	char duty[16] = {0};
	char dutyCycle[128] = {0};
	ISP_CTRL_PARAM_S stIspCtrlParam;

	CVI_ISP_GetCtrlParam(ViPipe, &stIspCtrlParam);
	snprintf(dutyCycle, sizeof(dutyCycle), "%s/pwm%d/duty_cycle", aeDCIrisPwmPath, stIspCtrlParam.u32PwmNumber);
	fd_duty = open(dutyCycle, O_RDWR);
	if (fd_duty < 0) {
		printf("open duty_cycle error\n");
		return CVI_FAILURE;
	}
	snprintf(duty, sizeof(duty), "%d", pwm);
	ret = write(fd_duty, duty, strlen(duty));
	if (ret < 0) {
		printf("Set fd_duty error\n");
		close(fd_duty);
		return CVI_FAILURE;
	}
	close(fd_duty);
	return CVI_SUCCESS;
}
#else
CVI_S32 AE_DCIrisPwmCtrl(VI_PIPE ViPipe, CVI_S32 pwm)
{
	(void)ViPipe;
	(void)pwm;
	return CVI_SUCCESS;
}
#endif

void AE_RunDCIris(VI_PIPE ViPipe, ISP_OP_TYPE_E aeMode)
{
	static CVI_U16 lastLuma[AE_SENSOR_NUM];
	static CVI_U32 lastPwmDuty[AE_SENSOR_NUM];
	static CVI_U8 maxBvCnt[AE_SENSOR_NUM];
	static CVI_U8 equLumaCnt[AE_SENSOR_NUM];
	SAE_INFO *tmpAeInfo;
	CVI_U32 u32CurPwmDuty = 0;
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);
	CVI_U32 u32MaxPwmDuty = pstAeDcIrisAttrInfo[sID]->u32MaxPwmDuty * 1000;
	static CVI_U8 PwmReset[AE_SENSOR_NUM];
	static CVI_U32 smoothDown[AE_SENSOR_NUM];

	if (!bHasDcIris[sID])
		return;

	if (pstAeIrisAttrInfo[sID]->enOpType == OP_TYPE_MANUAL) {
		if (pstAeIrisAttrInfo[sID]->enIrisStatus == ISP_IRIS_OPEN) {
			AE_DCIrisPwmCtrl(ViPipe, u32MaxPwmDuty);
		} else if (pstAeIrisAttrInfo[sID]->enIrisStatus == ISP_IRIS_CLOSE) {
			AE_DCIrisPwmCtrl(ViPipe, pstAeDcIrisAttrInfo[sID]->u32MinPwmDuty * 1000);
		} else if (pstAeIrisAttrInfo[sID]->enIrisStatus == ISP_IRIS_KEEP) {
			AE_DCIrisPwmCtrl(ViPipe, pstAeIrisAttrInfo[sID]->stMIAttr.u32HoldValue * 1000);
		}
		return;
	}

	if (aeMode == OP_TYPE_MANUAL)
		return;

	AE_GetCurrentInfo(sID, &tmpAeInfo);

	if (tmpAeInfo->stApex[AE_LE].s16BVEntry == tmpAeInfo->s16MaxTVEntry - tmpAeInfo->s16MinISOEntry ||
		(tmpAeInfo->bWDRMode && (tmpAeInfo->stApex[AE_SE].s16BVEntry == tmpAeInfo->s16MaxTVEntry))) {
		if (bUseDcIris[sID] == CVI_FALSE)
			maxBvCnt[sID]++;
		if (maxBvCnt[sID] > 5) {
			bUseDcIris[sID] = CVI_TRUE;
		}
	}
	if (bUseDcIris[sID]) {
		//AE_CalculateFrameLuma(sID);
		//AE_AdjustTargetY(sID);
		CVI_S32 s32MaxSafeLuma = pstAeDcIrisAttrInfo[sID]->u32MaxSafeLuma;

		if (s32MaxSafeLuma > 0 && s32MaxSafeLuma < tmpAeInfo->u16AdjustTargetLuma[AE_LE] + 200) //anti-fool
			s32MaxSafeLuma = tmpAeInfo->u16AdjustTargetLuma[AE_LE] + 200;

		if (tmpAeInfo->u16FrameLuma[AE_LE] > s32MaxSafeLuma && s32MaxSafeLuma > 0) { //handle overshoot,
			if (tmpAeInfo->u16FrameLuma[AE_LE] < lastLuma[sID]) {   //and if s32MaxSafeLuma=0, not handle it
				smoothDown[sID]++;
			} else if (tmpAeInfo->u16FrameLuma[AE_LE] == lastLuma[sID] && smoothDown[sID] > 0) {
				smoothDown[sID]++;
			} else {
				smoothDown[sID] = 0;
			}
		} else {
			smoothDown[sID] = 0;
		}

		if (smoothDown[sID] > 1) {
			u32CurPwmDuty = pstAeDcIrisAttrInfo[sID]->u32ToSafePwmDuty * 1000;
			PwmReset[sID] = 1;
		} else {
			if (pstAeDcIrisAttrInfo[sID]->enIrisPIDMode == ISP_IRIS_DELTA_INTG_PID)
				u32CurPwmDuty = AE_DCIrisDeltaIntg_PID(sID, tmpAeInfo->u16FrameLuma[AE_LE],
					tmpAeInfo->u16AdjustTargetLuma[AE_LE], PwmReset[sID]);
			else if (pstAeDcIrisAttrInfo[sID]->enIrisPIDMode == ISP_IRIS_POSITION_PID)
				u32CurPwmDuty = AE_DCIrisPosition_PID(sID, tmpAeInfo->u16FrameLuma[AE_LE],
					tmpAeInfo->u16AdjustTargetLuma[AE_LE], PwmReset[sID]);
			else if (pstAeDcIrisAttrInfo[sID]->enIrisPIDMode == ISP_IRIS_DELTA_PID)
				u32CurPwmDuty = AE_DCIrisDelta_PID(sID, tmpAeInfo->u16FrameLuma[AE_LE],
					tmpAeInfo->u16AdjustTargetLuma[AE_LE], PwmReset[sID]);
			else
				u32CurPwmDuty = u32MaxPwmDuty;
			PwmReset[sID] = 0;
		}

		if (lastPwmDuty[sID] != u32CurPwmDuty) {
			AE_DCIrisPwmCtrl(ViPipe, u32CurPwmDuty);
		}
		lastPwmDuty[sID] = u32CurPwmDuty;
		if (u32CurPwmDuty >= pstAeDcIrisAttrInfo[sID]->u32OpenPwmDuty * 1000) {
			if (abs(tmpAeInfo->u16FrameLuma[AE_LE] - lastLuma[sID]) < 2) {
				if (equLumaCnt[sID]++ > 5) {
					bUseDcIris[sID] = CVI_FALSE;
					maxBvCnt[sID] = 0;
					equLumaCnt[sID] = 0;
					AE_DCIrisPwmCtrl(ViPipe, u32MaxPwmDuty);
				}
			} else {
				equLumaCnt[sID] = 0;
			}
		} else {
			equLumaCnt[sID] = 0;
		}
		lastLuma[sID] = tmpAeInfo->u16FrameLuma[AE_LE];
		if (AE_GetDebugMode(sID) == AE_DEBUG_PRINT_IRIS_INFO) {
			printf("fid = [%d] maxBvCnt =%d frameLuma = [%d] TargetLuma =%d setPwm = [%d]\n\n",
			tmpAeInfo->u32frmCnt, maxBvCnt[sID], tmpAeInfo->u16FrameLuma[AE_LE],
			tmpAeInfo->u16AdjustTargetLuma[AE_LE], u32CurPwmDuty);
		}
	}
}

CVI_BOOL AE_IsDCIris(CVI_U8 sID)
{
	CVI_BOOL isDcIris = 0;

	sID = AE_CheckSensorID(sID);
#ifdef ENABLE_IRIS_CONTROL
	isDcIris = (pstAeIrisAttrInfo[sID]->bEnable &&
		pstAeIrisAttrInfo[sID]->enIrisType == ISP_IRIS_DC_TYPE) ? 1 : 0;
#endif
	return isDcIris;
}

CVI_BOOL AE_IsUseDcIris(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
#ifdef ENABLE_IRIS_CONTROL
	return bUseDcIris[sID];
#else
	return 0;
#endif
}

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic pop
#endif