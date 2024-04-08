#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>

#include "cvi_base.h"
#include "cvi_vi.h"
#include "cvi_sys.h"
#include "vi_ioctl.h"
#include "cvi_isp.h"


extern int get_v4l2_fd(int pipe);
#define CHECK_VI_NULL_PTR(ptr)							\
	do {									\
		if (ptr == NULL) {						\
			CVI_TRACE_VI(CVI_DBG_ERR, " Invalid null pointer\n");	\
			return CVI_ERR_VI_INVALID_NULL_PTR;			\
		}								\
	} while (0)

#define GET_BASE_VADDR(ip_info_id)				\
	do {							\
		paddr = ip_info_list[ip_info_id].str_addr;	\
		size = ip_info_list[ip_info_id].size;		\
		vaddr = CVI_SYS_Mmap(paddr, size);		\
	} while (0)

#define CLEAR_ACCESS_CNT(addr_ofs)				\
	do {							\
		val = vaddr + addr_ofs;				\
		*val = 0x1;					\
	} while (0)

#define SET_BITS(addr_ofs, val_ofs)				\
	do {							\
		val = vaddr + addr_ofs;				\
		data = *val | (0x1 << val_ofs);			\
		*val = data;					\
	} while (0)

#define CLEAR_BITS(addr_ofs, val_ofs)				\
	do {							\
		val = vaddr + addr_ofs;				\
		data = *val & (~(0x1 << val_ofs));		\
		*val = data;					\
	} while (0)

#define SET_REGISTER_COMMON(addr_ofs, val_ofs, on_off)		\
	do {							\
		if (!on_off) {/* off */				\
			CLEAR_BITS(addr_ofs, val_ofs);		\
		} else {/* on */				\
			SET_BITS(addr_ofs, val_ofs);		\
		}						\
	} while (0)

#define GET_REGISTER_COMMON(addr_ofs, val_ofs, ret)		\
	do {							\
		val = vaddr + addr_ofs;				\
		data = *val;					\
		ret = (data >> val_ofs) & 0x1;			\
	} while (0)

#define WAIT_IP_DISABLE(addr_ofs, val_ofs)			\
	do {							\
		do {						\
			usleep(1);				\
			val = vaddr + addr_ofs;			\
			data = *val;				\
		} while (((data >> val_ofs) & 0x1) != 0);	\
	} while (0)

#define SET_SW_MODE_ENABLE(addr_ofs, val_ofs, on_off)		\
	SET_REGISTER_COMMON(addr_ofs, val_ofs, on_off)

#define SET_SW_MODE_MEM_SEL(addr_ofs, val_ofs, mem_id)		\
	SET_REGISTER_COMMON(addr_ofs, val_ofs, mem_id)

#define FPRINTF_VAL()							\
	do {								\
		val = vaddr + offset;					\
		fprintf(fp, "\t\"%s\": {\n", name);			\
		fprintf(fp, "\t\t\"length\": %u,\n", length);		\
		fprintf(fp, "\t\t\"lut\": [\n");			\
		fprintf(fp, "\t\t\t");					\
		for (CVI_U32 i = 0 ; i < length; i++) {			\
			if (i == length - 1) {				\
				fprintf(fp, "%u\n", *val);		\
			} else if (i % 16 == 15) {			\
				fprintf(fp, "%u,\n\t\t\t", *val);	\
			} else {					\
				fprintf(fp, "%u,\t", *val);		\
			}						\
		}							\
		fprintf(fp, "\t\t]\n\t},\n");				\
	} while (0)

#define FPRINTF_VAL2()							\
	do {								\
		val = vaddr + offset;					\
		fprintf(fp, "\t\"%s\": {\n", name);			\
		fprintf(fp, "\t\t\"length\": %u,\n", length);		\
		fprintf(fp, "\t\t\"lut\": [\n");			\
		fprintf(fp, "\t\t\t");					\
		for (CVI_U32 i = 0 ; i < length; i++) {			\
			if (i == length - 1) {				\
				fprintf(fp, "%u\n", *(val++));		\
			} else if (i % 16 == 15) {			\
				fprintf(fp, "%u,\n\t\t\t", *(val++));	\
			} else {					\
				fprintf(fp, "%u,\t", *(val++));		\
			}						\
		}							\
		fprintf(fp, "\t\t]\n\t},\n");				\
	} while (0)

#define FPRINTF_TBL(data_tbl)						\
	do {								\
		fprintf(fp, "\t\"%s\": {\n", name);			\
		fprintf(fp, "\t\t\"length\": %u,\n", length);		\
		fprintf(fp, "\t\t\"lut\": [\n");			\
		fprintf(fp, "\t\t\t");					\
		for (CVI_U32 i = 0 ; i < length; i++) {			\
			if (i == length - 1) {				\
				fprintf(fp, "%u\n", data_tbl[i]);	\
			} else if (i % 16 == 15) {			\
				fprintf(fp, "%u,\n\t\t\t", data_tbl[i]);\
			} else {					\
				fprintf(fp, "%u,\t", data_tbl[i]);	\
			}						\
		}							\
		fprintf(fp, "\t\t]\n\t},\n");				\
	} while (0)

#define FPRINTF_FTBL(data_tbl)						\
	do {								\
		fprintf(fp, "\t\"%s\": {\n", name);			\
		fprintf(fp, "\t\t\"length\": %u,\n", length);		\
		fprintf(fp, "\t\t\"lut\": [\n");			\
		fprintf(fp, "\t\t\t");					\
		for (CVI_U32 i = 0 ; i < length; i++) {			\
			if (i == length - 1) {				\
				fprintf(fp, "%f\n", data_tbl[i]);	\
			} else if (i % 16 == 15) {			\
				fprintf(fp, "%f,\n\t\t\t", data_tbl[i]);\
			} else {					\
				fprintf(fp, "%f,\t", data_tbl[i]);	\
			}						\
		}							\
		fprintf(fp, "\t\t]\n\t},\n");				\
	} while (0)

#define DUMP_LUT_BASE(data_tbl, data_mask, r_addr, r_trig, r_data)	\
	do {								\
		for (CVI_U32 i = 0 ; i < length; i++) {			\
			val = vaddr + r_addr;				\
			*val = i;					\
			usleep(1);					\
									\
			val = vaddr + r_trig;				\
			data = (*val | (0x1 << 31));			\
			*val = data;					\
			usleep(1);					\
									\
			val = vaddr + r_data;				\
			data = *val;					\
			data_tbl[i] = (data & data_mask);		\
		}							\
	} while (0)

#define DUMP_LUT_COMMON(data_tbl, data_mask, sw_mode, r_addr, r_trig, r_data)	\
	do {									\
		val = vaddr + sw_mode;						\
		data = 0x1;							\
		*val = data;							\
										\
		DUMP_LUT_BASE(data_tbl, data_mask, r_addr, r_trig, r_data);	\
										\
		val = vaddr + sw_mode;						\
		data = 0x0;							\
		*val = data;							\
	} while (0)

struct reg_tbl {
	int addr_ofs;
	int val_ofs;
	int data;
	int mask;
};

struct gamma_tbl {
	enum IP_INFO_GRP ip_info_id;
	char name[16];
	int length;
	struct reg_tbl enable;
	struct reg_tbl shdw_sel;
	struct reg_tbl force_clk_enable;
	struct reg_tbl prog_en;
	struct reg_tbl raddr;
	struct reg_tbl rdata_r;
	struct reg_tbl rdata_gb;
};

static void _dump_gamma_table(FILE *fp, struct ip_info *ip_info_list, struct gamma_tbl *tbl)
{
	CVI_U32 paddr;
	CVI_U32 size;
	CVI_VOID *vaddr;
	volatile CVI_U32 *val;

	CVI_U32 length = tbl->length;
	CVI_U32 data;
	CVI_CHAR name[32];

	CVI_U32 *data_gamma_r = calloc(1, sizeof(CVI_U32) * length);
	CVI_U32 *data_gamma_g = calloc(1, sizeof(CVI_U32) * length);
	CVI_U32 *data_gamma_b = calloc(1, sizeof(CVI_U32) * length);

	GET_BASE_VADDR(tbl->ip_info_id);
	SET_REGISTER_COMMON(tbl->prog_en.addr_ofs, tbl->prog_en.val_ofs, 1);

	if (tbl->ip_info_id == IP_INFO_ID_RGBGAMMA ||
		tbl->ip_info_id == IP_INFO_ID_YGAMMA ||
		tbl->ip_info_id == IP_INFO_ID_DCI) {
		CVI_U8 r_sel = 0;

		GET_REGISTER_COMMON(tbl->prog_en.addr_ofs, 4, r_sel);
		CVI_TRACE_VI(CVI_DBG_INFO, "mem[%d] work, mem[%d] IDLE\n", r_sel, r_sel ^ 0x1);
		SET_REGISTER_COMMON(tbl->raddr.addr_ofs, tbl->raddr.val_ofs, r_sel ^ 0x1);
	}

	for (CVI_U32 i = 0 ; i < length; i++) {
		val = vaddr + tbl->raddr.addr_ofs;
		data = (*val & (~tbl->raddr.mask)) | i;
		*val = data;

		val = vaddr + tbl->rdata_r.addr_ofs;
		data = (*val | (0x1 << tbl->rdata_r.val_ofs));
		*val = data;

		val = vaddr + tbl->rdata_r.addr_ofs;
		data = *val;
		data_gamma_r[i] = (data & tbl->rdata_r.mask);

		val = vaddr + tbl->rdata_gb.addr_ofs;
		data = *val;
		data_gamma_g[i] = (data & tbl->rdata_gb.mask);
		data_gamma_b[i] = ((data >> tbl->rdata_gb.val_ofs) & tbl->rdata_gb.mask);
	}

	SET_REGISTER_COMMON(tbl->prog_en.addr_ofs, tbl->prog_en.val_ofs, 0);

	memset(name, 0, sizeof(name));
	strcat(strcat(name, tbl->name), "_r");
	FPRINTF_TBL(data_gamma_r);

	memset(name, 0, sizeof(name));
	strcat(strcat(name, tbl->name), "_g");
	FPRINTF_TBL(data_gamma_g);

	memset(name, 0, sizeof(name));
	strcat(strcat(name, tbl->name), "_b");
	FPRINTF_TBL(data_gamma_b);

	CVI_SYS_Munmap(vaddr, size);
	free(data_gamma_r);
	free(data_gamma_g);
	free(data_gamma_b);
}

CVI_S32 dump_register(VI_PIPE ViPipe, FILE *fp, VI_DUMP_REGISTER_TABLE_S *pstRegTbl)
{
	CHECK_VI_NULL_PTR(fp);
	CHECK_VI_NULL_PTR(pstRegTbl);

	CVI_S32 s32Ret = CVI_SUCCESS;
	struct ip_info *ip_info_list;

	ip_info_list = calloc(1, sizeof(struct ip_info) * IP_INFO_ID_MAX);

	CVI_U32 paddr;
	CVI_U32 size;
	CVI_VOID *vaddr;
	CVI_U32 *val;
	ISP_PUB_ATTR_S stPubAttr;
	CVI_U32 ip_info_id;
	CVI_U32 offset;
	CVI_U32 length;
	CVI_U32 data;
	CVI_CHAR name[32];
	CVI_CHAR cmd[128];
	CVI_S32 fd = get_v4l2_fd( ViPipe);

	s32Ret = CVI_ISP_GetPubAttr(ViPipe, &stPubAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "Get Pub Attr fail\n");
		return s32Ret;
	}

	s32Ret = vi_get_ip_dump_list(fd, ip_info_list);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_get_ip_dump_list ioctl\n");
		free(ip_info_list);
		return s32Ret;
	}

	/* stop tuning update */
	sprintf(cmd, "echo %d,1,1,1 > /sys/module/soph_ispv4l2/parameters/tuning_dis", ViPipe + 1);
	system(cmd);

	/* In the worst case, have to wait two frames to stop tuning update. */
	usleep(80 * 1000);
	/* start of file */
	fprintf(fp, "{\n");
	/* dump isp moudle register */
	for (CVI_U32 i = 0; i < IP_INFO_ID_MAX; i++) {
		CVI_TRACE_VI(CVI_DBG_INFO, "%d\n", i);

		GET_BASE_VADDR(i);
		val = vaddr;

		fprintf(fp, "\t\"0x%08X\": {\n", paddr);
		for (CVI_U32 j = 0 ; j < (size / 0x4); j++) {
			fprintf(fp, "\t\t\"h%02x\": %u,\n", (j * 4), *(val++));
		}
		fprintf(fp, "\t\t\"size\": %u\n\t},\n", size);

		CVI_SYS_Munmap(vaddr, size);
	}

	/* dump look up table */
	// DPC
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "DPC\n");

		CVI_U8 enable = 0;
		// CVI_U8 shdw_sel = 0;
		CVI_U8 force_clk_enable = 0;

		CVI_U8 idx = (stPubAttr.enWDRMode == WDR_MODE_2To1_LINE) ? 2 : 1;
		CVI_U32 *data_dpc = NULL;
		CVI_CHAR name_list[2][16] = {"dpc_le_bp_tbl", "dpc_se_bp_tbl"};
		enum IP_INFO_GRP ip_info_id_list[2] = {IP_INFO_ID_DPC0, IP_INFO_ID_DPC1};

		length = 2047;

		for (CVI_U8 i = 0; i < idx; i++) {
			GET_BASE_VADDR(ip_info_id_list[i]);
			data_dpc = calloc(1, sizeof(CVI_U32) * length);

			GET_REGISTER_COMMON(0x8, 0, enable);
			// GET_REGISTER_COMMON(0x4, 0, shdw_sel);
			GET_REGISTER_COMMON(0x8, 8, force_clk_enable);

			SET_REGISTER_COMMON(0x8, 0, 0); // reg_dpc_enable
			// SET_REGISTER_COMMON(0x4, 0, 0); // reg_shdw_read_sel
			SET_REGISTER_COMMON(0x8, 8, 0); // reg_force_clk_enable
			WAIT_IP_DISABLE(0x8, 0);
			SET_REGISTER_COMMON(0x44, 31, 1); // reg_dpc_mem_prog_mode

			DUMP_LUT_BASE(data_dpc, 0xFFFFFF, 0x48, 0x4C, 0x4C);

			SET_REGISTER_COMMON(0x44, 31, 0); // reg_dpc_mem_prog_mode
			SET_REGISTER_COMMON(0x8, 8, force_clk_enable); // reg_force_clk_enable
			// SET_REGISTER_COMMON(0x4, 0, shdw_sel); // reg_shdw_read_sel
			SET_REGISTER_COMMON(0x8, 0, enable); // reg_dpc_enable

			snprintf(name, sizeof(name), name_list[i]);
			FPRINTF_TBL(data_dpc);

			free(data_dpc);
			CVI_SYS_Munmap(vaddr, size);
		}
	}

	// MLSC
	// NO INDIRECT LUT
	// use the isp tun buffer
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "MLSC\n");
		MLSC_GAIN_LUT_S *MlscGainLut = &pstRegTbl->MlscGainLut;

		length = 1369;
		if (MlscGainLut->RGain && MlscGainLut->GGain && MlscGainLut->BGain) {
			snprintf(name, sizeof(name), "mlsc_gain_r");
			FPRINTF_TBL(MlscGainLut->RGain);

			snprintf(name, sizeof(name), "mlsc_gain_g");
			FPRINTF_TBL(MlscGainLut->GGain);

			snprintf(name, sizeof(name), "mlsc_gain_b");
			FPRINTF_TBL(MlscGainLut->BGain);
		} else {
			CVI_TRACE_VI(CVI_DBG_INFO, "MLSC is no data\n");
		}
	}

	// RGB_GAMMA
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "RGB_GAMMA\n");

		struct gamma_tbl rgb_gamma = {
			.ip_info_id = IP_INFO_ID_RGBGAMMA,
			.name = "rgb_gamma",
			.length = 256,
			.enable = {
				.addr_ofs = 0x0,
				.val_ofs = 0,
			},
			.shdw_sel = {
				.addr_ofs = 0x0,
				.val_ofs = 1,
			},
			.force_clk_enable = {
				.addr_ofs = 0x0,
				.val_ofs = 2,
			},
			.prog_en = {
				.addr_ofs = 0x4,
				.val_ofs = 8,
			},
			.raddr = {
				.addr_ofs = 0x14,
				.mask = 0xFF,
				.val_ofs = 12,
			},
			.rdata_r = {
				.addr_ofs = 0x18,
				.val_ofs = 31,
				.mask = 0xFFF,
			},
			.rdata_gb = {
				.addr_ofs = 0x1C,
				.val_ofs = 16,
				.mask = 0xFFF,
			},
		};

		_dump_gamma_table(fp, ip_info_list, &rgb_gamma);
	}

	//Y_GAMMA
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "Y_GAMMA\n");

		struct gamma_tbl y_gamma = {
			.ip_info_id = IP_INFO_ID_YGAMMA,
			.name = "y_gamma",
			.length = 256,
			.enable = {
				.addr_ofs = 0x0,
				.val_ofs = 0,
			},
			.shdw_sel = {
				.addr_ofs = 0x0,
				.val_ofs = 1,
			},
			.force_clk_enable = {
				.addr_ofs = 0x0,
				.val_ofs = 2,
			},
			.prog_en = {
				.addr_ofs = 0x4,
				.val_ofs = 8,
			},
			.raddr = {
				.addr_ofs = 0x14,
				.mask = 0xFF,
				.val_ofs = 12,
			},
			.rdata_r = {
				.addr_ofs = 0x18,
				.val_ofs = 31,
				.mask = 0xFFFF,
			},
			.rdata_gb = {
				.addr_ofs = 0x1C,
				.val_ofs = 16,
				.mask = 0xFFF,
			},
		};

		_dump_gamma_table(fp, ip_info_list, &y_gamma);
	}

	// CLUT
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "CLUT\n");
		ip_info_id = IP_INFO_ID_CLUT;

		CVI_U32 r_idx = 17;
		CVI_U32 g_idx = 17;
		CVI_U32 b_idx = 17;
		CVI_U32 rgb_idx = 0;
		CVI_U32 *data_clut_r = NULL;
		CVI_U32 *data_clut_g = NULL;
		CVI_U32 *data_clut_b = NULL;
		CVI_U8 enable = 0;
		CVI_U8 shdw_sel = 0;
		// CVI_U8 force_clk_enable = 0;

		length = r_idx * g_idx * b_idx;
		data_clut_r = calloc(1, sizeof(CVI_U32) * length);
		data_clut_g = calloc(1, sizeof(CVI_U32) * length);
		data_clut_b = calloc(1, sizeof(CVI_U32) * length);

		GET_BASE_VADDR(ip_info_id);
		GET_REGISTER_COMMON(0x0, 0, enable);
		GET_REGISTER_COMMON(0x0, 1, shdw_sel);
		// GET_REGISTER_COMMON(0x0, 2, force_clk_enable);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_clut_enable
		SET_REGISTER_COMMON(0x0, 1, 0); // reg_clut_shdw_sel
		// SET_REGISTER_COMMON(0x0, 2, 0); // reg_force_clk_enable
		WAIT_IP_DISABLE(0x0, 0);
		SET_REGISTER_COMMON(0x0, 3, 1); // reg_prog_en

		for (CVI_U32 i = 0 ; i < b_idx; i++) {
			for (CVI_U32 j = 0 ; j < g_idx; j++) {
				for (CVI_U32 k = 0 ; k < r_idx; k++) {
					rgb_idx = i * g_idx * r_idx + j * r_idx + k;

					val = vaddr + 0x04; // reg_sram_r_idx/reg_sram_g_idx/reg_sram_b_idx
					data = (i << 16) | (j << 8) | k;
					*val = data;
					usleep(1);

					val = vaddr + 0x0C; // reg_sram_rd
					data = (0x1 << 31);
					*val = data;
					usleep(1);

					val = vaddr + 0x0C; // reg_sram_rdata
					data = *val;
					usleep(1);

					data_clut_r[rgb_idx] = (data >> 20) & 0x3FF;
					data_clut_g[rgb_idx] = (data >> 10) & 0x3FF;
					data_clut_b[rgb_idx] = data & 0x3FF;
				}
			}
		}

		SET_REGISTER_COMMON(0x0, 3, 0); // reg_prog_en
		// SET_REGISTER_COMMON(0x0, 2, force_clk_enable); // reg_force_clk_enable
		SET_REGISTER_COMMON(0x0, 1, shdw_sel); // reg_clut_shdw_sel
		SET_REGISTER_COMMON(0x0, 0, enable); // reg_clut_enable

		snprintf(name, sizeof(name), "clut_r");
		FPRINTF_TBL(data_clut_r);

		snprintf(name, sizeof(name), "clut_g");
		FPRINTF_TBL(data_clut_g);

		snprintf(name, sizeof(name), "clut_b");
		FPRINTF_TBL(data_clut_b);

		CVI_SYS_Munmap(vaddr, size);
		free(data_clut_r);
		free(data_clut_g);
		free(data_clut_b);
	}

	// LTM
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "LTM\n");
		ip_info_id = IP_INFO_ID_LTM;

		// CVI_U8 enable = 0;
		// CVI_U8 shdw_sel = 0;
		// CVI_U8 force_clk_enable = 0;
		CVI_U8 sel = 0;

		GET_BASE_VADDR(ip_info_id);

		GET_REGISTER_COMMON(0x60, 0, sel);
		CVI_TRACE_VI(CVI_DBG_INFO, "mem[%d] work, mem[%d] IDLE\n", sel, sel ^ 0x1);
		SET_REGISTER_COMMON(0x34, 14, sel ^ 1); // reg_lut_dbg_rsel
		// GET_REGISTER_COMMON(0x0, 0, enable);
		// GET_REGISTER_COMMON(0x0, 5, shdw_sel);
		// GET_REGISTER_COMMON(0x0, 31, force_clk_enable);

		// SET_REGISTER_COMMON(0x0, 0, 0); // reg_ltm_enable
		// SET_REGISTER_COMMON(0x0, 5, 0); // reg_shdw_read_sel
		// SET_REGISTER_COMMON(0x0, 31, 0); // reg_force_clk_enable
		// WAIT_IP_DISABLE(0x0, 0);

		// dark tone
		length = 257;
		CVI_U32 *data_dtone_curve = calloc(1, sizeof(CVI_U32) * length);

		SET_REGISTER_COMMON(0x34, 17, 1); // reg_lut_prog_en_dark
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x34;
			data = (*val & ~(0x3FF)) | i; // reg_lut_dbg_raddr[0,9]
			data = (data | (0x1 << 15)); // reg_lut_dbg_read_en_1t
			*val = data;
			usleep(1);

			val = vaddr + 0x4C; // reg_lut_dbg_rdata
			data = *val;
			data_dtone_curve[i] = data;
		}
		SET_REGISTER_COMMON(0x34, 17, 0); // reg_lut_prog_en_dark
		val = vaddr + 0x44; //reg_dark_lut_max
		data_dtone_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_dtone_curve");
		FPRINTF_TBL(data_dtone_curve);
		free(data_dtone_curve);

		// bright tone
		length = 513;

		CVI_U32 *data_btone_curve = calloc(1, sizeof(CVI_U32) * length);

		SET_REGISTER_COMMON(0x34, 16, 1); // reg_lut_prog_en_bright
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x34;
			data = (*val & ~(0x3FF)) | i; // reg_lut_dbg_raddr[0,9]
			data = (data | (0x1 << 15)); // reg_lut_dbg_read_en_1t
			*val = data;
			usleep(1);

			val = vaddr + 0x4C;
			data = *val;
			data_btone_curve[i] = data;
		}

		SET_REGISTER_COMMON(0x34, 16, 0); // reg_lut_prog_en_bright
		val = vaddr + 0x40; //reg_bright_lut_max
		data_btone_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_btone_curve");
		FPRINTF_TBL(data_btone_curve);
		free(data_btone_curve);

		// global tone
		length = 769;

		CVI_U32 *data_global_curve = calloc(1, sizeof(CVI_U32) * length);

		SET_REGISTER_COMMON(0x34, 18, 1); // reg_lut_prog_en_global
		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x34;
			data = (*val & ~(0x3FF)) | i; // reg_lut_dbg_raddr[0,9]
			data = (data | (0x1 << 15)); // reg_lut_dbg_read_en_1t
			*val = data;
			usleep(1);

			val = vaddr + 0x4C;
			data = *val;
			data_global_curve[i] = data;
		}
		SET_REGISTER_COMMON(0x34, 18, 0); // reg_lut_prog_en_global
		val = vaddr + 0x48; //reg_global_lut_max
		data_global_curve[length - 1] = *val;
		snprintf(name, sizeof(name), "ltm_global_curve");
		FPRINTF_TBL(data_global_curve);
		free(data_global_curve);

		// SET_REGISTER_COMMON(0x0, 31, force_clk_enable); // reg_force_clk_enable
		// SET_REGISTER_COMMON(0x0, 5, shdw_sel); // reg_shdw_read_sel
		// SET_REGISTER_COMMON(0x0, 0, enable); // reg_ltm_enable

		CVI_SYS_Munmap(vaddr, size);
	}

	// CA_CP
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "CA_CP\n");
		length = 256;
		ip_info_id = IP_INFO_ID_CA;

		CVI_U8 enable = 0;
		CVI_U8 shdw_sel = 0;
		CVI_U8 ca_cp_mode = 0;
		CVI_U32 *data_cacp_y = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_cacp_u = calloc(1, sizeof(CVI_U32) * length);
		CVI_U32 *data_cacp_v = calloc(1, sizeof(CVI_U32) * length);

		GET_BASE_VADDR(ip_info_id);
		GET_REGISTER_COMMON(0x0, 0, enable);
		GET_REGISTER_COMMON(0x0, 4, shdw_sel);
		GET_REGISTER_COMMON(0x0, 1, ca_cp_mode);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_cacp_enable
		SET_REGISTER_COMMON(0x0, 4, 0); // reg_cacp_shdw_read_sel
		WAIT_IP_DISABLE(0x0, 0);
		SET_REGISTER_COMMON(0x0, 3, 1); // reg_cacp_mem_sw_mode

		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x0C;
			data = (*val & ~0xFF) | i;
			*val = data;
			usleep(1);

			val = vaddr + 0x0C;
			data = (*val | (0x1 << 31));
			*val = data;
			usleep(1);

			val = vaddr + 0x10;
			data = *val;

			if (ca_cp_mode) {
				data_cacp_y[i] = ((data >> 16) & 0xFF);
				data_cacp_u[i] = ((data >> 8) & 0xFF);
				data_cacp_v[i] = (data & 0xFF);
			} else {
				data_cacp_y[i] = (data & 0x7FF);
			}
		}

		SET_REGISTER_COMMON(0x0, 3, 0);  // reg_cacp_mem_sw_mode
		SET_REGISTER_COMMON(0x0, 4, shdw_sel); // reg_cacp_shdw_read_sel
		SET_REGISTER_COMMON(0x0, 0, enable); // reg_cacp_enable

		if (ca_cp_mode) {
			snprintf(name, sizeof(name), "ca_cp_y");
			FPRINTF_TBL(data_cacp_y);
			snprintf(name, sizeof(name), "ca_cp_u");
			FPRINTF_TBL(data_cacp_u);
			snprintf(name, sizeof(name), "ca_cp_v");
			FPRINTF_TBL(data_cacp_v);
		} else {
			snprintf(name, sizeof(name), "ca_y_ratio");
			FPRINTF_TBL(data_cacp_y);
		}

		CVI_SYS_Munmap(vaddr, size);
		free(data_cacp_y);
		free(data_cacp_u);
		free(data_cacp_v);
	}

	// YNR
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "YNR\n");
		ip_info_id = IP_INFO_ID_YNR;

		CVI_U8 shdw_sel = 0;

		GET_BASE_VADDR(ip_info_id);
		GET_REGISTER_COMMON(0x0, 0, shdw_sel);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_shadow_rd_sel
		CLEAR_ACCESS_CNT(0x8);

		length = 6;
		offset = 0x00C;
		snprintf(name, sizeof(name), "ynr_ns0_luma_th");
		FPRINTF_VAL2();

		length = 5;
		offset = 0x024;
		snprintf(name, sizeof(name), "ynr_ns0_slope");
		FPRINTF_VAL2();

		length = 6;
		offset = 0x038;
		snprintf(name, sizeof(name), "ynr_ns0_offset");
		FPRINTF_VAL2();

		length = 6;
		offset = 0x050;
		snprintf(name, sizeof(name), "ynr_ns1_luma_th");
		FPRINTF_VAL2();

		length = 5;
		offset = 0x068;
		snprintf(name, sizeof(name), "ynr_ns1_slope");
		FPRINTF_VAL2();

		length = 6;
		offset = 0x07C;
		snprintf(name, sizeof(name), "ynr_ns1_offset");
		FPRINTF_VAL2();

		length = 16;
		offset = 0x098;
		snprintf(name, sizeof(name), "ynr_motion_lut");
		FPRINTF_VAL2();

		length = 16;
		offset = 0x260;
		snprintf(name, sizeof(name), "ynr_res_mot_lut");
		FPRINTF_VAL2();

		length = 64;
		offset = 0x200;
		snprintf(name, sizeof(name), "ynr_weight_lut");
		FPRINTF_VAL();

		SET_REGISTER_COMMON(0x0, 0, shdw_sel); // reg_shadow_rd_sel

		CVI_SYS_Munmap(vaddr, size);
	}

	// YCURVE
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "YCURVE\n");
		length = 64;
		ip_info_id = IP_INFO_ID_YCURVE;

		CVI_U8 enable = 0;
		CVI_U8 shdw_sel = 0;
		CVI_U8 force_clk_enable = 0;
		CVI_U32 *data_ycurve = calloc(1, sizeof(CVI_U32) * length);

		GET_BASE_VADDR(ip_info_id);
		GET_REGISTER_COMMON(0x0, 0, enable);
		GET_REGISTER_COMMON(0x0, 1, shdw_sel);
		GET_REGISTER_COMMON(0x0, 2, force_clk_enable);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_ycur_enable
		SET_REGISTER_COMMON(0x0, 1, 0); // reg_ycur_shdw_sel
		SET_REGISTER_COMMON(0x0, 2, 0); // reg_force_clk_enable
		WAIT_IP_DISABLE(0x0, 0);
		SET_REGISTER_COMMON(0x4, 8, 1); // reg_ycur_prog_en

		CVI_U8 r_sel = 0;

		GET_REGISTER_COMMON(0x4, 4, r_sel);
		CVI_TRACE_VI(CVI_DBG_INFO, "mem[%d] work, mem[%d] IDLE\n", r_sel, r_sel ^ 0x1);
		SET_REGISTER_COMMON(0x14, 12, r_sel ^ 0x1);

		for (CVI_U32 i = 0 ; i < length; i++) {
			val = vaddr + 0x14;
			data = (*val & ~(0x3F)) | i;
			*val = data;
			usleep(1);

			val = vaddr + 0x18;
			data = (*val | (0x1 << 31));
			*val = data;
			usleep(1);

			val = vaddr + 0x18;
			data = *val;
			data_ycurve[i] = (data & 0xFF);
		}

		SET_REGISTER_COMMON(0x4, 8, 0); // reg_ycur_prog_en
		SET_REGISTER_COMMON(0x0, 2, force_clk_enable); // reg_force_clk_enable
		SET_REGISTER_COMMON(0x0, 1, shdw_sel); // reg_ycur_shdw_sel
		SET_REGISTER_COMMON(0x0, 0, enable); // reg_ycur_enable

		snprintf(name, sizeof(name), "ycurve");
		FPRINTF_TBL(data_ycurve);

		free(data_ycurve);
		CVI_SYS_Munmap(vaddr, size);
	}

	// DCI_GAMMA
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "DCI_GAMMA\n");

		struct gamma_tbl dci_gamma = {
			.ip_info_id = IP_INFO_ID_DCI,
			.name = "dci_gamma",
			.length = 256,
			.enable = {
				.addr_ofs = 0x0C,
				.val_ofs = 0,
			},
			.shdw_sel = {
				.addr_ofs = 0x14,
				.val_ofs = 4,
			},
			.force_clk_enable = {
				.addr_ofs = 0x0C,
				.val_ofs = 8,
			},
			.prog_en = {
				.addr_ofs = 0x204,
				.val_ofs = 8,
			},
			.raddr = {
				.addr_ofs = 0x214,
				.mask = 0xFF,
				.val_ofs = 12,
			},
			.rdata_r = {
				.addr_ofs = 0x218,
				.val_ofs = 31,
				.mask = 0xFFF,
			},
			.rdata_gb = {
				.addr_ofs = 0x21C,
				.val_ofs = 16,
				.mask = 0xFFF,
			},
		};

		_dump_gamma_table(fp, ip_info_list, &dci_gamma);
	}

	// BNR
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "BNR\n");

		CVI_U8 idx = (stPubAttr.enWDRMode == WDR_MODE_2To1_LINE) ? 2 : 1;
		CVI_CHAR bnr_intensity_sel_name_list[2][21] = {"bnr_le_intensity_sel", "bnr_se_intensity_sel"};
		CVI_CHAR bnr_weight_lut_name_list[2][18] = {"bnr_le_weight_lut", "bnr_se_weight_lut"};
		enum IP_INFO_GRP ip_info_id_list[2] = {IP_INFO_ID_BNR0, IP_INFO_ID_BNR1};

		for (CVI_U8 i = 0; i < idx; i++) {
			ip_info_id = ip_info_id_list[i];
			GET_BASE_VADDR(ip_info_id);
			CLEAR_ACCESS_CNT(0x8);

			length = 8;
			offset = 0x148;
			snprintf(name, sizeof(name), bnr_intensity_sel_name_list[i]);
			FPRINTF_VAL();

			length = 256;
			offset = 0x228;
			snprintf(name, sizeof(name), bnr_weight_lut_name_list[i]);
			FPRINTF_VAL();
		}
	}

	// TEAISP_BNR
	{
		CVI_TRACE_VI(CVI_DBG_INFO, "TEAISP_BNR\n");

		struct isp_tuning_cfg *tun_buf_info;
		struct cvi_vip_isp_fe_cfg *fe_addr;

		tun_buf_info = calloc(1, sizeof(struct isp_tuning_cfg));
		s32Ret = vi_get_tun_addr(fd, tun_buf_info);
		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_VI(CVI_DBG_ERR, "vi_get_tun_addr ioctl\n");
			free(tun_buf_info);
			return s32Ret;
		}

		length = 6;

		tun_buf_info->fe_vir[ViPipe] =
				CVI_SYS_Mmap(tun_buf_info->fe_addr[ViPipe], sizeof(struct cvi_vip_isp_fe_cfg));
		fe_addr = (struct cvi_vip_isp_fe_cfg *)(tun_buf_info->fe_vir[ViPipe]);
		CVI_FLOAT *bnr_param_list = calloc(1, sizeof(CVI_FLOAT) * length);
		struct cvi_vip_teaisp_bnr_config *pre_fe_teaisp_bnr = &fe_addr->tun_cfg[0].bnr;
		volatile CVI_FLOAT *temp = NULL;
		CVI_U32 temp_f = 0;
		// use the isp tun buffer
		{
			temp = (volatile CVI_FLOAT *) &temp_f;

			temp_f = pre_fe_teaisp_bnr->blc;
			bnr_param_list[0] = *temp;
			temp_f = pre_fe_teaisp_bnr->coeff_a;
			bnr_param_list[1] = *temp * 1000;
			temp_f = pre_fe_teaisp_bnr->coeff_b;
			bnr_param_list[2] = *temp * 1000;
			temp_f = pre_fe_teaisp_bnr->filter_motion_str_2d;
			bnr_param_list[3] = *temp;
			temp_f = pre_fe_teaisp_bnr->filter_static_str_2d;
			bnr_param_list[4] = *temp;
			temp_f = pre_fe_teaisp_bnr->filter_str_3d;
			bnr_param_list[5] = *temp;

			snprintf(name, sizeof(name), "teaisp_bnr");
			FPRINTF_FTBL(bnr_param_list);
		}
		free(tun_buf_info);
		free(bnr_param_list);
	}

	/* end of file */
	fprintf(fp, "\t\"end\": {}\n");
	fprintf(fp, "}");
	/* start tuning update */
	system("echo 0,0,0,0 > /sys/module/soph_ispv4l2/parameters/tuning_dis");

	free(ip_info_list);

	return CVI_SUCCESS;
}

