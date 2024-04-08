
#include <string.h>
#include "isp_ipc_sdk_wrap.h"
#include "isp_ipc_common.h"
#include "isp_ipc_server_cmd.h"
#include "isp_mgr_buf.h"
#include "cvi_bin.h"

#ifdef ENABLE_ISP_IPC

CVI_U32 CVI_BIN_GetBinTotalLenWrap(void)
{
	VI_PIPE ViPipe = 0; // default 0 is avilable

	CVI_U32 binSize = 0;
	ISP_IPC_MSG_S msg;

	if (isp_mgr_buf_is_client(ViPipe)) {

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_BIN_GET_TOTAL_LEN;
		msg.arg_len = 0;
		msg.res_len = sizeof(CVI_U32);

		client_send_cmd_to_server(&msg, NULL, (CVI_U8 *) &binSize);

	} else {
		binSize = CVI_BIN_GetBinTotalLen();
	}

	return binSize;
}

CVI_S32 CVI_BIN_ExportBinDataWrap(CVI_U8 *pu8Buffer, CVI_U32 u32DataLength)
{
#define DESC_SIZE 624 // from cvi_bin.c !!!!!!!

	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE ViPipe = 0; // default 0 is avilable

	ISP_IPC_MSG_S msg;

	if (isp_mgr_buf_is_client(ViPipe)) {

		CVI_BIN_HEADER	*pstBinHeader;
		CVI_BIN_EXTRA_S *ptBinExtra;

		pstBinHeader = (CVI_BIN_HEADER *) pu8Buffer;
		ptBinExtra = (CVI_BIN_EXTRA_S *) calloc(1, sizeof(CVI_BIN_EXTRA_S));

		memcpy(ptBinExtra, &pstBinHeader->extraInfo, sizeof(CVI_BIN_EXTRA_S));

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_BIN_EXPORT_BIN;
		msg.arg_len = 0;
		msg.res_len = u32DataLength;

		s32Ret = client_send_cmd_to_server(&msg, NULL, pu8Buffer);

		memcpy(&pstBinHeader->extraInfo.Author, ptBinExtra->Author, 32);
		memcpy(&pstBinHeader->extraInfo.Time, ptBinExtra->Time, 32);
		memcpy(&pstBinHeader->extraInfo.Desc, ptBinExtra->Desc, DESC_SIZE);

		free(ptBinExtra);

	} else {
		s32Ret = CVI_BIN_ExportBinData(pu8Buffer, u32DataLength);
	}

	return s32Ret;
}

CVI_S32 CVI_BIN_GetBinNameWrap(CVI_CHAR *binName)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE ViPipe = 0; // default 0 is avilable

	char szBinFileName[128] = {'\0'};
	ISP_IPC_MSG_S msg;

	if (isp_mgr_buf_is_client(ViPipe)) {

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_BIN_GET_BIN_NAME;
		msg.arg_len = 0;
		msg.res_len = 128;

		s32Ret = client_send_cmd_to_server(&msg, NULL, (CVI_U8 *) szBinFileName);

		sprintf(binName, "%s", szBinFileName);

	} else {
		s32Ret = CVI_BIN_GetBinName(binName);
	}

	return s32Ret;
}

CVI_S32 CVI_BIN_ImportBinDataWrap(CVI_U8 *pu8Buffer, CVI_U32 u32DataLength)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE ViPipe = 0; // default 0 is avilable

	ISP_IPC_MSG_S msg;

	if (isp_mgr_buf_is_client(ViPipe)) {

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_BIN_IMPORT_BIN;
		msg.arg_len = u32DataLength;
		msg.res_len = 0;

		s32Ret = client_send_cmd_to_server(&msg, pu8Buffer, NULL);

	} else {
		s32Ret = CVI_BIN_ImportBinData(pu8Buffer, u32DataLength);
	}

	return s32Ret;
}

#endif

