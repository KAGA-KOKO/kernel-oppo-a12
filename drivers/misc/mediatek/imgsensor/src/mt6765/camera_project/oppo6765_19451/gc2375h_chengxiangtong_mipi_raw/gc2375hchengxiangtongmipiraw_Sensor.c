 /*
 *
 * Filename:
 * ---------
 *     gc2375hmipi_Sensor.c
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     Source code of Sensor driver
 *
 *
 *-----------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 */

#define PFX "GC2375H"
#define LOG_INF(format, args...)    \
		pr_debug(PFX "[%s] " format, __func__, ##args)
#define LOG_DBG(format, args...)    \
		pr_debug(PFX "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...)    \
		pr_err(PFX "[%s] " format, __func__, ##args)


#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include <linux/of_gpio.h>

#include "gc2375hchengxiangtongmipiraw_Sensor.h"

/* Xuegui.Bao@Camera.Driver, 2018/10/12, add for [summer hardwareinfo bringup] */
#include "cam_cal_define.h"
/* Tao.Li@Camera.Driver, 2019/12/25, add for [Cola hardwareinfo bringup] */
#define HARDWARE_INFO  1
#ifdef HARDWARE_INFO
#include <linux/hardware_info.h>
extern int hardwareinfo_set_prop(int cmd, const char *name);
#endif

#define LOG_INF(format, args...)    \
		pr_debug(PFX "[%s] " format, __func__, ##args)
#define LOG_DBG(format, args...)    \
		pr_debug(PFX "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...)    \
		pr_err(PFX "[%s] " format, __func__, ##args)

/**************************** Modify end *****************************/

static DEFINE_SPINLOCK(imgsensor_drv_lock);


static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = GC2375H_CHENGXIANGTONG_SENSOR_ID, /* record sensor id defined in Kd_imgsensor.h */
	.checksum_value = 0xf7375923,    /* checksum value for Camera Auto Test */

	.pre = {
		.pclk = 39000000,         /* record different mode's pclk */
		.linelength = 1040,       /* record different mode's linelength */
		.framelength = 1250,      /* record different mode's framelength */
		.startx = 0,               /* record different mode's startx of grabwindow */
		.starty = 0,               /* record different mode's starty of grabwindow */
		.grabwindow_width = 1600,  /* record different mode's width of grabwindow */
		.grabwindow_height = 1200, /* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario */
		.mipi_data_lp2hs_settle_dc = 85, /* unit , ns */
		/*.mipi_pixel_rate = 624000000,*/
		/* following for GetDefaultFramerateByScenario() */
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 39000000,
		.linelength = 1040,
		.framelength = 1250,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		/*.mipi_pixel_rate = 624000000,*/
		.max_framerate = 300,
	},
	.cap1 = {
	/* capture for PIP 24fps relative information, capture1 mode must use same framelength,
	linelength with Capture mode for shutter calculate */
		.pclk = 39000000,
		.linelength = 1040,
		.framelength = 1250,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		/*.mipi_pixel_rate = 624000000,*/
		.max_framerate = 300,
	},
	.normal_video = {
		.pclk = 39000000,
		.linelength = 1040,
		.framelength = 1250,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		/*.mipi_pixel_rate = 624000000,*/
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 39000000,
		.linelength = 1040,
		.framelength = 1250,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		/*.mipi_pixel_rate = 624000000,*/
		.max_framerate = 300,
	},
	.slim_video = {
		.pclk = 39000000,
		.linelength = 1040,
		.framelength = 1250,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		/*.mipi_pixel_rate = 624000000,*/
		.max_framerate = 300,
	},
    .custom1 = { //30.24-->30.00 //24.007
        .pclk = 39000000,
        .linelength = 1040,
        .framelength = 1562,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 1600,
        .grabwindow_height = 1200,
        .mipi_data_lp2hs_settle_dc = 85,
        /*.mipi_pixel_rate = 624000000,*/
        .max_framerate = 240,
    },

	.margin = 2,            /* sensor framelength & shutter margin */
	.min_shutter = 1,       /* min shutter */
	.max_frame_length = 0x3fff, /* max framelength by sensor register's limitation */
	/* shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2 */
	.ae_shut_delay_frame = 0,
	/* sensor gain delay frame for AE cycle, 2 frame with ispGain_delay-sensor_gain_delay=2-0=2 */
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2, /* isp gain delay frame for AE cycle */
	.ihdr_support = 0,      /* 1, support; 0,not support */
	.ihdr_le_firstline = 0, /* 1,le first ; 0, se first */
	.sensor_mode_num = 6,   /* support sensor mode num */
	.frame_time_delay_frame = 1,//The delay frame of setting frame length

	.cap_delay_frame = 2,   /* enter capture delay frame num */
	.pre_delay_frame = 2,   /* enter preview delay frame num */
	.video_delay_frame = 2, /* enter video delay frame num */
	.hs_video_delay_frame = 2, /* enter high speed video  delay frame num */
	.slim_video_delay_frame = 2,/* enter slim video delay frame num */
	.custom1_delay_frame = 2,
	.isp_driving_current = ISP_DRIVING_4MA, /* mclk driving current */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI, /* sensor_interface_type */
	.mipi_sensor_type = MIPI_OPHY_NCSI2, /* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	/* 0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL */
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,
#if defined(GC2375H_MIRROR_NORMAL)
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R, /* sensor output first pixel color */
#elif defined(GC2375H_MIRROR_H)
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr, /* sensor output first pixel color */
#elif defined(GC2375H_MIRROR_V)
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb, /* sensor output first pixel color */
#elif defined(GC2375H_MIRROR_HV)
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B, /* sensor output first pixel color */
#else
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R, /* sensor output first pixel color */
#endif
	.mclk = 24, /* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_1_LANE, /* mipi lane num */
	.i2c_addr_table = {0x2e, 0xff}, /* record sensor support all write id addr, only supprt 4must end with 0xff */
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,    /* mirrorflip information */
	/* IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture,
	Video,High Speed Video, Slim Video */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3ED,   /* current shutter */
	.gain = 0x40,       /* current gain */
	.dummy_pixel = 0,   /* current dummypixel */
	.dummy_line = 0,    /* current dummyline */
	.current_fps = 300, /* full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	/* auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker */
	.autoflicker_en = KAL_FALSE,
	/* test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output */
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW, /*current scenario id */
	.ihdr_en = 0, /* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x2e, /* record current sensor's i2c write id */
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[6] = {
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200}, /* Preview */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200}, /* capture */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200}, /* video */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200}, /* hight speed video */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200},  /* slim video */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200}  /* slim video */
};

#define MULTI_WRITE 1
#if MULTI_WRITE
#define I2C_BUFFER_LEN 1024
#else
#define I2C_BUFFER_LEN 2
#endif

static kal_uint16 gc2375_table_write_cmos_sensor(
					kal_uint16 *para, kal_uint32 len)
{
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend, IDX;
	kal_uint16 addr = 0, addr_last = 0, data;

	tosend = 0;
	IDX = 0;
	while (len > IDX) {
		addr = para[IDX];

		{
			puSendCmd[tosend++] = (char)(addr & 0xFF);
			data = para[IDX + 1];
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;

		}
#if MULTI_WRITE
		if ((I2C_BUFFER_LEN - tosend) < 2 ||
			len == IDX ||
			addr != addr_last) {
			LOG_INF("sp2509_table_write_cmos_sensor multi write tosend=%d, len=%d, IDX=%d, addr=%x, addr_last=%x\n", tosend, len, IDX, addr, addr_last);
			iBurstWriteReg_multi(puSendCmd, tosend,
				imgsensor.i2c_write_id,
				2, imgsensor_info.i2c_speed);

			tosend = 0;
		}
#else
		iWriteRegI2C(puSendCmd, 2, imgsensor.i2c_write_id);
		tosend = 0;
#endif
	}
	return 0;
}

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[1] = {(char)(addr & 0xFF)};

	iReadRegI2C(pu_send_cmd, 1, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[2] = {(char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 2, imgsensor.i2c_write_id);
}

#if defined(GC2375HMIPI_USE_OTP)
static struct gc2375h_otp gc2375h_otp_info;

static kal_uint8 gc2375h_read_otp(kal_uint8 addr)
{
	kal_uint8 value;

	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0xd5, addr);
	write_cmos_sensor(0xf3, 0x20);
	value = read_cmos_sensor(0xd7);

	return value;
}

static void gc2375h_gcore_read_otp_info(void)
{
	kal_uint8 flag;

	memset(&gc2375h_otp_info, 0, sizeof(gc2375h_otp_info));

	flag = gc2375h_read_otp(0x00);
	LOG_INF("GC2375H_OTP: flag = 0x%x\n", flag);

	gc2375h_otp_info.wb_flag = flag & 0x03;
	gc2375h_otp_info.ob_flag = (flag >> 2) & 0x03;

	/* WB */
	if (0x01 == gc2375h_otp_info.wb_flag) {
		LOG_INF("GC2375H_OTP_WB: Valid");

		gc2375h_otp_info.r_gain = gc2375h_read_otp(0x08);
		gc2375h_otp_info.g_gain = gc2375h_read_otp(0x10);
		gc2375h_otp_info.b_gain = gc2375h_read_otp(0x18);
		gc2375h_otp_info.g_r_gain_l = gc2375h_read_otp(0x20);
		gc2375h_otp_info.b_g_gain_l = gc2375h_read_otp(0x28);

		LOG_INF("GC2375H_OTP_WB: r_gain = 0x%x\n", gc2375h_otp_info.r_gain);
		LOG_INF("GC2375H_OTP_WB: g_gain = 0x%x\n", gc2375h_otp_info.g_gain);
		LOG_INF("GC2375H_OTP_WB: b_gain = 0x%x\n", gc2375h_otp_info.b_gain);
		LOG_INF("GC2375H_OTP_WB: g_r_gain_l = 0x%x\n", gc2375h_otp_info.g_r_gain_l);
		LOG_INF("GC2375H_OTP_WB: b_g_gain_l = 0x%x\n", gc2375h_otp_info.b_g_gain_l);
	} else {
		LOG_INF("GC2375H_OTP_WB: Invalid!");
	}

	/* OB */
	if (0x01 == gc2375h_otp_info.ob_flag) {
		LOG_INF("GC2375H_OTP_OB:Valid");

		gc2375h_otp_info.g1_slope = gc2375h_read_otp(0x30);
		gc2375h_otp_info.r1_slope = gc2375h_read_otp(0x38);
		gc2375h_otp_info.b2_slope = gc2375h_read_otp(0x40);
		gc2375h_otp_info.g2_slope = gc2375h_read_otp(0x48);
		gc2375h_otp_info.ob_slope = gc2375h_read_otp(0x50);

		LOG_INF("GC2375H_OTP_OB: g1_slope = 0x%x\n", gc2375h_otp_info.g1_slope);
		LOG_INF("GC2375H_OTP_OB: r1_slope = 0x%x\n", gc2375h_otp_info.r1_slope);
		LOG_INF("GC2375H_OTP_OB: b2_slope = 0x%x\n", gc2375h_otp_info.b2_slope);
		LOG_INF("GC2375H_OTP_OB: g2_slope = 0x%x\n", gc2375h_otp_info.g2_slope);
		LOG_INF("GC2375H_OTP_OB: ob_slope = 0x%x\n", gc2375h_otp_info.ob_slope);
	} else {
		LOG_INF("GC2375H_OTP_OB:Invalid!");
	}
}

static void gc2375h_gcore_update_wb(void)
{
	if (0x01 == gc2375h_otp_info.wb_flag) {
		write_cmos_sensor(0xfe, 0x00);
		write_cmos_sensor(0xb8, gc2375h_otp_info.g_gain);
		write_cmos_sensor(0xb9, gc2375h_otp_info.r_gain);
		write_cmos_sensor(0xba, gc2375h_otp_info.b_gain);
		write_cmos_sensor(0xbb, gc2375h_otp_info.g_gain);
		write_cmos_sensor(0xbe, gc2375h_otp_info.g_r_gain_l);
		write_cmos_sensor(0xbf, gc2375h_otp_info.b_g_gain_l);
		write_cmos_sensor(0xfe, 0x00);
	}
}

static void gc2375h_gcore_update_ob(void)
{
	if (0x01 == gc2375h_otp_info.ob_flag) {
		write_cmos_sensor(0xfe, 0x01);
		write_cmos_sensor(0x41, gc2375h_otp_info.g1_slope);
		write_cmos_sensor(0x42, gc2375h_otp_info.r1_slope);
		write_cmos_sensor(0x43, gc2375h_otp_info.b2_slope);
		write_cmos_sensor(0x44, gc2375h_otp_info.g2_slope);
		write_cmos_sensor(0x45, gc2375h_otp_info.ob_slope);
		write_cmos_sensor(0xfe, 0x00);
	}
}

static void gc2375h_gcore_update_otp(void)
{
	gc2375h_gcore_update_wb();
	gc2375h_gcore_update_ob();
}

static void gc2375h_gcore_enable_otp(kal_uint8 state)
{
	kal_uint8 otp_clk, otp_en;

	otp_clk = read_cmos_sensor(0xfc);
	otp_en = read_cmos_sensor(0xd4);
	if (state) {
		otp_clk = otp_clk | 0x10;
		otp_en = otp_en | 0x80;
		mdelay(5);
		write_cmos_sensor(0xfc, otp_clk); /* 0xfc[4]: OTP_CLK_en */
		write_cmos_sensor(0xd4, otp_en);  /* 0xd4[7]: OTP_en */

		LOG_INF("GC2375H_OTP: Enable OTP!\n");
	} else {
		otp_en = otp_en & 0x7f;
		otp_clk = otp_clk & 0xef;
		mdelay(5);
		write_cmos_sensor(0xd4, otp_en);
		write_cmos_sensor(0xfc, otp_clk);

		LOG_INF("GC2375H_OTP: Disable OTP!\n");
	}
}

void gc2375h_gcore_identify_otp(void)
{
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0xf7, 0x01);
	write_cmos_sensor(0xf8, 0x0c);
	write_cmos_sensor(0xf9, 0x42);
	write_cmos_sensor(0xfa, 0x88);
	write_cmos_sensor(0xfc, 0x8e);
	gc2375h_gcore_enable_otp(otp_open);
	gc2375h_gcore_read_otp_info();
	gc2375h_gcore_enable_otp(otp_close);
}
#endif

static void set_dummy(void)
{
	kal_uint32 vb = 0;
	kal_uint32  basic_line = 1224;

	//return; //for test
	vb = imgsensor.frame_length - basic_line;
	vb = vb < 16 ? 16 : vb;
	vb = vb > 8191 ? 8191 : vb;
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0x07, (vb >> 8) & 0x1F);
	write_cmos_sensor(0x08, vb & 0xFF);
	LOG_INF("vb = %d, frame_length = %d \n", vb, imgsensor.frame_length);
}    /*    set_dummy  */

static kal_uint32 return_sensor_id(void)
{
	kal_uint32 res0,res1;

	res0 = read_cmos_sensor(0xf0);
	res1 = read_cmos_sensor(0xf1);

	LOG_INF("[gc2375]res0 = 0x0%x,[gc2375]res1 = 0x0%x",res0,res1);
	return (((read_cmos_sensor(0xf0) << 8) | read_cmos_sensor(0xf1))+1);
}

/*
static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;
	unsigned long flags;

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length =
		(frame_length > imgsensor.min_frame_length) ?
		frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
	    imgsensor.frame_length = imgsensor_info.max_frame_length;
	    imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
	    imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}*/

/*************************************************************************
* FUNCTION
*    set_shutter
*
* DESCRIPTION
*    This function set e-shutter of sensor to change exposure time.
*
* PARAMETERS
*    iShutter : exposured lines
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	/* kal_uint16 realtime_fps = 0; */
	/* kal_uint32 frame_length = 0; */

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	/* if shutter bigger than frame_length, should extend frame length first */
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ?
		(imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (shutter == (imgsensor.frame_length - 1))
		shutter += 1;

	if (shutter > 16383)
		shutter = 16383;
	if (shutter < 1)
		shutter = 1;

	/* Update Shutter */
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0x03, (shutter >> 8) & 0x3f);
	write_cmos_sensor(0x04, shutter & 0xff);

	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);
}
static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min framelength should enable = %d\n", framerate,min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;

	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	//dummy_line = frame_length - imgsensor.min_frame_length;
	//if (dummy_line < 0)
	//imgsensor.dummy_line = 0;
	//else
	//imgsensor.dummy_line = dummy_line;
	//imgsensor.frame_length = frame_length + imgsensor.dummy_line;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
	{
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);

	set_dummy();
}    /*    set_max_framerate  */

static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	spin_lock(&imgsensor_drv_lock);
	/*Change frame time*/
	if (frame_length > 1) {
		imgsensor.frame_length = frame_length;
    }
	if (shutter > imgsensor.frame_length - imgsensor_info.margin) {
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	}
	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	}
	spin_unlock(&imgsensor_drv_lock);

	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;

	if (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) {
		shutter = (imgsensor_info.max_frame_length - imgsensor_info.margin);
	}

	shutter = (shutter >> 1) << 1;
	imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;
	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk * 10 /
			(imgsensor.line_length * imgsensor.frame_length);
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
	}

    {
        kal_uint32 vb = 0;
        kal_uint32  basic_line = 1224;

        //return; //for test
        vb = imgsensor.frame_length - basic_line;

        vb = vb < 16 ? 16 : vb;
        vb = vb > 8191 ? 8191 : vb;
        write_cmos_sensor(0xfe, 0x00);

        write_cmos_sensor(0x07, (vb >> 8) & 0x1F);
        write_cmos_sensor(0x08, vb & 0xFF);
        LOG_INF("vb = %d, frame_length = %d \n", vb, imgsensor.frame_length);
    }

	/* Update Shutter*/
	if (shutter > 16383) {
		shutter = 16383;
	}
	if (shutter < 1) {
		shutter = 1;
	}

	//Update Shutter
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0x03, (shutter>>8) & 0x3F);
	write_cmos_sensor(0x04, shutter & 0xFF);
	LOG_INF("Add for N3D! shutterlzl =%d, framelength =%d\n", shutter, imgsensor.frame_length);
}


/*
static kal_uint16 gain2reg(const kal_uint16 gain)
{
    kal_uint16 reg_gain = 0x0000;

    reg_gain = ((gain / BASEGAIN) << 4) + ((gain % BASEGAIN) * 16 / BASEGAIN);
    reg_gain = reg_gain & 0xFFFF;
    return (kal_uint16)reg_gain;
}*/

/*************************************************************************
* FUNCTION
*    set_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    iGain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/

#define ANALOG_GAIN_1 64   /* 1.00x */
#define ANALOG_GAIN_2 92   /* 1.43x */
#define ANALOG_GAIN_3 128  /* 2.00x */
#define ANALOG_GAIN_4 182  /* 2.84x */
#define ANALOG_GAIN_5 254  /* 3.97x */
#define ANALOG_GAIN_6 363  /* 5.68x */
#define ANALOG_GAIN_7 521  /* 8.14x */
#define ANALOG_GAIN_8 725  /* 11.34x */
#define ANALOG_GAIN_9 1038 /* 16.23x */

static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 iReg, temp;

	iReg = gain;

	if (iReg < 0x40)
		iReg = 0x40;

	if ((ANALOG_GAIN_1 <= iReg) && (iReg < ANALOG_GAIN_2)) {
		write_cmos_sensor(0x20, 0x0b);
		write_cmos_sensor(0x22, 0x0c);
		write_cmos_sensor(0x26, 0x0e);
		/* analog gain */
		write_cmos_sensor(0xb6, 0x00);
		temp = iReg;
		write_cmos_sensor(0xb1, temp >> 6);
		write_cmos_sensor(0xb2, (temp << 2) & 0xfc);
		LOG_INF("GC2375HMIPI analogic gain 1x, GC2375HMIPI add pregain = %d\n", temp);
	} else if ((ANALOG_GAIN_2 <= iReg) && (iReg < ANALOG_GAIN_3)) {
		write_cmos_sensor(0x20, 0x0c);
		write_cmos_sensor(0x22, 0x0e);
		write_cmos_sensor(0x26, 0x0e);
		/* analog gain */
		write_cmos_sensor(0xb6, 0x01);
		temp = 64 * iReg / ANALOG_GAIN_2;
		write_cmos_sensor(0xb1, temp >> 6);
		write_cmos_sensor(0xb2, (temp << 2) & 0xfc);
		LOG_INF("GC2375HMIPI analogic gain 1.43x , GC2375HMIPI add pregain = %d\n", temp);
	} else if ((ANALOG_GAIN_3 <= iReg) && (iReg < ANALOG_GAIN_4)) {
		write_cmos_sensor(0x20, 0x0c);
		write_cmos_sensor(0x22, 0x0e);
		write_cmos_sensor(0x26, 0x0e);
		/* analog gain */
		write_cmos_sensor(0xb6, 0x02);
		temp = 64 * iReg / ANALOG_GAIN_3;
		write_cmos_sensor(0xb1, temp >> 6);
		write_cmos_sensor(0xb2, (temp << 2) & 0xfc);
		LOG_INF("GC2375HMIPI analogic gain 2x , GC2375HMIPI add pregain = %d\n", temp);
	} else if ((ANALOG_GAIN_4 <= iReg) && (iReg < ANALOG_GAIN_5)) {
		write_cmos_sensor(0x20, 0x0c);
		write_cmos_sensor(0x22, 0x0e);
		write_cmos_sensor(0x26, 0x0e);
		/* analog gain */
		write_cmos_sensor(0xb6, 0x03);
		temp = 64 * iReg / ANALOG_GAIN_4;
		write_cmos_sensor(0xb1, temp >> 6);
		write_cmos_sensor(0xb2, (temp << 2) & 0xfc);
		LOG_INF("GC2375HMIPI analogic gain 2.84x , GC2375HMIPI add pregain = %d\n", temp);
	} else if ((ANALOG_GAIN_5 <= iReg) && (iReg < ANALOG_GAIN_6)) {
		write_cmos_sensor(0x20, 0x0c);
		write_cmos_sensor(0x22, 0x0e);
		write_cmos_sensor(0x26, 0x0e);
		/* analog gain */
		write_cmos_sensor(0xb6, 0x04);
		temp = 64 * iReg / ANALOG_GAIN_5;
		write_cmos_sensor(0xb1, temp >> 6);
		write_cmos_sensor(0xb2, (temp << 2) & 0xfc);
		LOG_INF("GC2375HMIPI analogic gain 3.97x , GC2375HMIPI add pregain = %d\n", temp);
	} else if ((ANALOG_GAIN_6 <= iReg) && (iReg < ANALOG_GAIN_7)) {
		write_cmos_sensor(0x20, 0x0e);
		write_cmos_sensor(0x22, 0x0e);
		write_cmos_sensor(0x26, 0x0e);
		/* analog gain */
		write_cmos_sensor(0xb6, 0x05);
		temp = 64 * iReg / ANALOG_GAIN_6;
		write_cmos_sensor(0xb1, temp >> 6);
		write_cmos_sensor(0xb2, (temp << 2) & 0xfc);
		LOG_INF("GC2375HMIPI analogic gain 5.68x , GC2375HMIPI add pregain = %d\n", temp);
	} else if ((ANALOG_GAIN_7 <= iReg) && (iReg < ANALOG_GAIN_8)) {
		write_cmos_sensor(0x20, 0x0c);
		write_cmos_sensor(0x22, 0x0c);
		write_cmos_sensor(0x26, 0x0e);
		/* analog gain */
		write_cmos_sensor(0xb6, 0x06);
		temp = 64 * iReg / ANALOG_GAIN_7;
		write_cmos_sensor(0xb1, temp >> 6);
		write_cmos_sensor(0xb2, (temp << 2) & 0xfc);
		LOG_INF("GC2375HMIPI analogic gain 8.14x , GC2375HMIPI add pregain = %d\n", temp);
	} else if ((ANALOG_GAIN_8 <= iReg) && (iReg < ANALOG_GAIN_9)) {
		write_cmos_sensor(0x20, 0x0e);
		write_cmos_sensor(0x22, 0x0e);
		write_cmos_sensor(0x26, 0x0e);
		/* analog gain */
		write_cmos_sensor(0xb6, 0x07);
		temp = 64 * iReg / ANALOG_GAIN_8;
		write_cmos_sensor(0xb1, temp >> 6);
		write_cmos_sensor(0xb2, (temp << 2) & 0xfc);
		LOG_INF("GC2375HMIPI analogic gain 11.34x , GC2375HMIPI add pregain = %d\n", temp);
	} else {
		write_cmos_sensor(0x20, 0x0c);
		write_cmos_sensor(0x22, 0x0e);
		write_cmos_sensor(0x26, 0x0e);
		/* analog gain */
		write_cmos_sensor(0xb6, 0x08);
		temp = 64 * iReg / ANALOG_GAIN_9;
		write_cmos_sensor(0xb1, temp >> 6);
		write_cmos_sensor(0xb2, (temp << 2) & 0xfc);
		LOG_INF("GC2375HMIPI analogic gain 16.23x , GC2375HMIPI add pregain = %d\n", temp);
	}
	return gain;
}
/*
static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n", le, se, gain);
}
*/
/*
static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);

}

*/
/*************************************************************************
* FUNCTION
*    night_mode
*
* DESCRIPTION
*    This function night mode of sensor.
*
* PARAMETERS
*    bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void night_mode(kal_bool enable)
{
/* No Need to implement this function */
}

#if MULTI_WRITE
static kal_uint16 addr_data_pair_init_gc2375[] = {
	0xfe, 0x00,
	0xfe, 0x00,
	0xfe, 0x00,
	0xf7, 0x01,
	0xf8, 0x0c,
	0xf9, 0x42,
	0xfa, 0x88,
	0xfc, 0x8e,
	0xfe, 0x00,
	0x88, 0x03,
	0x03, 0x04,
	0x04, 0x65,
	0x05, 0x02,
	0x06, 0x5a,
	0x07, 0x00,
	0x08, 0x1a,
	0x09, 0x00,
	0x0a, 0x04,
	0x0b, 0x00,
	0x0c, 0x14,
	0x0d, 0x04,
	0x0e, 0xb8,
	0x0f, 0x06,
	0x10, 0x48,
	0x17, GC2375H_MIRROR,
	0x1c, 0x10,
	0x1d, 0x13,
	0x20, 0x0b,
	0x21, 0x6d,
	0x22, 0x0c,
	0x25, 0xc1,
	0x26, 0x0e,
	0x27, 0x22,
	0x29, 0x5f,
	0x2b, 0x88,
	0x2f, 0x12,
	0x38, 0x86,
	0x3d, 0x00,
	0xcd, 0xa3,
	0xce, 0x57,
	0xd0, 0x09,
	0xd1, 0xca,
	0xd2, 0x34,
	0xd3, 0xbb,
	0xd8, 0x60,
	0xe0, 0x08,
	0xe1, 0x1f,
	0xe4, 0xf8,
	0xe5, 0x0c,
	0xe6, 0x10,
	0xe7, 0xcc,
	0xe8, 0x02,
	0xe9, 0x01,
	0xea, 0x02,
	0xeb, 0x01,
	0x90, 0x01,
	0x92, 0x04,
	0x94, 0x04,
	0x95, 0x04,
	0x96, 0xb0,
	0x97, 0x06,
	0x98, 0x40,
	0x18, 0x02,
	0x1a, 0x18,
	0x28, 0x00,
	0x3f, 0x40,
	0x40, 0x26,
	0x41, 0x00,
	0x43, 0x03,
	0x4a, 0x00,
	0x4e, BLK_Select1_H,
	0x4f, BLK_Select1_L,
	0x66, BLK_Select2_H,
	0x67, BLK_Select2_L,
	0x68, 0x00,
	0xb0, 0x58,
	0xb1, 0x01,
	0xb2, 0x00,
	0xb6, 0x00,
	0xfe, 0x03,
	0x01, 0x03,
	0x02, 0x33,
	0x03, 0x90,
	0x04, 0x04,
	0x05, 0x00,
	0x06, 0x80,
	0x11, 0x2b,
	0x12, 0xd0,
	0x13, 0x07,
	0x15, 0x00,
	0x21, 0x08,
	0x22, 0x05,
	0x23, 0x13,
	0x24, 0x02,
	0x25, 0x13,
	0x26, 0x08,
	0x29, 0x06,
	0x2a, 0x08,
	0x2b, 0x08,
	0xfe, 0x00,
};
#endif

static void sensor_init(void)
{
	LOG_INF("E");
#if MULTI_WRITE
	LOG_INF("sensor_init multi write\n");
	gc2375_table_write_cmos_sensor(
		addr_data_pair_init_gc2375,
		sizeof(addr_data_pair_init_gc2375) / sizeof(kal_uint16));
#else
	LOG_INF("sensor_init normal write\n");
	/* System */
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0xf7, 0x01);
	write_cmos_sensor(0xf8, 0x0c);
	write_cmos_sensor(0xf9, 0x42);
	write_cmos_sensor(0xfa, 0x88);
	write_cmos_sensor(0xfc, 0x8e);
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0x88, 0x03);

	/* Analog */
	write_cmos_sensor(0x03, 0x04);
	write_cmos_sensor(0x04, 0x65);
	write_cmos_sensor(0x05, 0x02);
	write_cmos_sensor(0x06, 0x5a);
	write_cmos_sensor(0x07, 0x00);
	write_cmos_sensor(0x08, 0x1a);//0x10
	write_cmos_sensor(0x09, 0x00);
	write_cmos_sensor(0x0a, 0x04);
	write_cmos_sensor(0x0b, 0x00);
	write_cmos_sensor(0x0c, 0x14);
	write_cmos_sensor(0x0d, 0x04);
	write_cmos_sensor(0x0e, 0xb8);
	write_cmos_sensor(0x0f, 0x06);
	write_cmos_sensor(0x10, 0x48);
	write_cmos_sensor(0x17, GC2375H_MIRROR);
	write_cmos_sensor(0x1c, 0x10);
	write_cmos_sensor(0x1d, 0x13);
	write_cmos_sensor(0x20, 0x0b);
	write_cmos_sensor(0x21, 0x6d);
	write_cmos_sensor(0x22, 0x0c);
	write_cmos_sensor(0x25, 0xc1);
	write_cmos_sensor(0x26, 0x0e);
	write_cmos_sensor(0x27, 0x22);
	write_cmos_sensor(0x29, 0x5f);
	write_cmos_sensor(0x2b, 0x88);
	write_cmos_sensor(0x2f, 0x12);
	write_cmos_sensor(0x38, 0x86);
	write_cmos_sensor(0x3d, 0x00);
	write_cmos_sensor(0xcd, 0xa3);
	write_cmos_sensor(0xce, 0x57);
	write_cmos_sensor(0xd0, 0x09);
	write_cmos_sensor(0xd1, 0xca);
	write_cmos_sensor(0xd2, 0x34);
	write_cmos_sensor(0xd3, 0xbb);
	write_cmos_sensor(0xd8, 0x60);
	write_cmos_sensor(0xe0, 0x08);
	write_cmos_sensor(0xe1, 0x1f);
	write_cmos_sensor(0xe4, 0xf8);
	write_cmos_sensor(0xe5, 0x0c);
	write_cmos_sensor(0xe6, 0x10);
	write_cmos_sensor(0xe7, 0xcc);
	write_cmos_sensor(0xe8, 0x02);
	write_cmos_sensor(0xe9, 0x01);
	write_cmos_sensor(0xea, 0x02);
	write_cmos_sensor(0xeb, 0x01);

	/* Crop */
	write_cmos_sensor(0x90, 0x01);
	write_cmos_sensor(0x92, 0x04);
	write_cmos_sensor(0x94, 0x04);
	write_cmos_sensor(0x95, 0x04);
	write_cmos_sensor(0x96, 0xb0);
	write_cmos_sensor(0x97, 0x06);
	write_cmos_sensor(0x98, 0x40);

	/* BLK */
	write_cmos_sensor(0x18, 0x02);
	write_cmos_sensor(0x1a, 0x18);
	write_cmos_sensor(0x28, 0x00);
	write_cmos_sensor(0x3f, 0x40);
	write_cmos_sensor(0x40, 0x26);
	write_cmos_sensor(0x41, 0x00);
	write_cmos_sensor(0x43, 0x03);
	write_cmos_sensor(0x4a, 0x00);
	write_cmos_sensor(0x4e, BLK_Select1_H);
	write_cmos_sensor(0x4f, BLK_Select1_L);
	write_cmos_sensor(0x66, BLK_Select2_H);
	write_cmos_sensor(0x67, BLK_Select2_L);

	/* Dark sun */
	write_cmos_sensor(0x68, 0x00);

	/* Gain */
	write_cmos_sensor(0xb0, 0x58);
	write_cmos_sensor(0xb1, 0x01);
	write_cmos_sensor(0xb2, 0x00);
	write_cmos_sensor(0xb6, 0x00);

	/* MIPI */
	write_cmos_sensor(0xfe, 0x03);
	write_cmos_sensor(0x01, 0x03);
	write_cmos_sensor(0x02, 0x33);
	write_cmos_sensor(0x03, 0x90);
	write_cmos_sensor(0x04, 0x04);
	write_cmos_sensor(0x05, 0x00);
	write_cmos_sensor(0x06, 0x80);
	write_cmos_sensor(0x11, 0x2b);
	write_cmos_sensor(0x12, 0xd0);
	write_cmos_sensor(0x13, 0x07);
	write_cmos_sensor(0x15, 0x00);
	write_cmos_sensor(0x21, 0x08);
	write_cmos_sensor(0x22, 0x05);
	write_cmos_sensor(0x23, 0x13);
	write_cmos_sensor(0x24, 0x02);
	write_cmos_sensor(0x25, 0x13);
	write_cmos_sensor(0x26, 0x08);
	write_cmos_sensor(0x29, 0x06);
	write_cmos_sensor(0x2a, 0x08);
	write_cmos_sensor(0x2b, 0x08);
	write_cmos_sensor(0xfe, 0x00);
#endif
}

static void preview_setting(void)
{
	pr_debug("E!\n");
}

static void capture_setting(kal_uint16 currefps)
{
	pr_debug("E! currefps:%d\n", currefps);
}

static void normal_video_setting(kal_uint16 currefps)
{
	pr_debug("E! currefps:%d\n", currefps);
}

static void hs_video_setting(void)
{
	pr_debug("E\n");
}

static void slim_video_setting(void)
{
	pr_debug("E\n");
}

#if MULTI_WRITE
static kal_uint16 addr_data_pair_custom1_gc2375[] = {
	0xfe, 0x00,
	0x03, 0x04,
	0x04, 0x65,
	0x05, 0x02,
	0x06, 0x5a,
	0x07, 0x01,
	0x08, 0x52,
	0xfe, 0x00,
};
#endif

static void custom1_setting(void)
{
#if MULTI_WRITE
	LOG_INF("custom1_setting multi write\n");
	gc2375_table_write_cmos_sensor(
		addr_data_pair_custom1_gc2375,
		sizeof(addr_data_pair_custom1_gc2375) / sizeof(kal_uint16));
#else
	LOG_INF("custom1_setting normal write\n");
	write_cmos_sensor(0xfe, 0x00);
	write_cmos_sensor(0x03, 0x04);
	write_cmos_sensor(0x04, 0x65);
	write_cmos_sensor(0x05, 0x02);
	write_cmos_sensor(0x06, 0x5a);
	write_cmos_sensor(0x07, 0x01);
	write_cmos_sensor(0x08, 0x52);
	write_cmos_sensor(0xfe, 0x00);
#endif
}


static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);
	write_cmos_sensor(0xfe, 0x00);
	if (enable)
		write_cmos_sensor(0x8c, 0x11);
	else
		write_cmos_sensor(0x8c, 0x10);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    get_imgsensor_id
*
* DESCRIPTION
*    This function get the sensor ID
*
* PARAMETERS
*    *sensorID : return the sensor ID
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);

		i++;
		do {
			*sensor_id = return_sensor_id();
			if (*sensor_id == imgsensor_info.sensor_id) {
				/* Tao.Li@Camera.Driver, 2019/12/25, add for [Cola hardwareinfo bringup] */
				#ifdef HARDWARE_INFO
				hardwareinfo_set_prop(HARDWARE_BACK_SUB_CAM, "GC2375H");
				hardwareinfo_set_prop(HARDWARE_BACK_SUB_CAM_MOUDULE_ID, "chengxiangtong");
				#endif
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}
			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
			retry--;
		} while (retry > 0);
		retry = 1;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF */
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    open
*
* DESCRIPTION
*    This function initialize the registers of CMOS sensor
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0;


	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
	    do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
			retry--;
	    } while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;

#ifdef GC2375HMIPI_USE_OTP
	gc2375h_gcore_identify_otp();
#endif

	/* initail sequence write in  */
	sensor_init();

#ifdef GC2375HMIPI_USE_OTP
	gc2375h_gcore_update_otp();
#endif

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    close
*
* DESCRIPTION
*
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
	LOG_INF("E\n");

	/* No Need to implement this function */

	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*    This function start the sensor preview.
*
* PARAMETERS
*    *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	/* imgsensor.video_mode = KAL_FALSE; */
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    capture
*
* DESCRIPTION
*    This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
		/* PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M */
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
			imgsensor.current_fps, imgsensor_info.cap.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);
	return ERROR_NONE;
}

static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	/* imgsensor.current_fps = 300; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	return ERROR_NONE;
}

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	/* imgsensor.video_mode = KAL_TRUE; */
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	return ERROR_NONE;
}

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	return ERROR_NONE;
}
static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
									MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	imgsensor.pclk = imgsensor_info.custom1.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.custom1.linelength;
	imgsensor.frame_length = imgsensor_info.custom1.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom1_setting();
return ERROR_NONE;
}	/*	Custom1 */

static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight = imgsensor_info.slim_video.grabwindow_height;
	sensor_resolution->SensorCustom1Width  = imgsensor_info.custom1.grabwindow_width;
	sensor_resolution->SensorCustom1Height     = imgsensor_info.custom1.grabwindow_height;

	return ERROR_NONE;
}

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
	MSDK_SENSOR_INFO_STRUCT *sensor_info,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	/* sensor_info->SensorVideoFrameRate = imgsensor_info.normal_video.max_framerate/10;*/
	/* not use */
	/* sensor_info->SensorStillCaptureFrameRate= imgsensor_info.cap.max_framerate/10; */
	/* not use */
	/* imgsensor_info->SensorWebCamCaptureFrameRate= imgsensor_info.v.max_framerate;*/
	/* not use */

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; /* inverse with datasheet */
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;
	sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;
	/* The frame of setting shutter default 0 for TG int */
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	/* The frame of setting sensor gain */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  /* 0 is default 1x */
	sensor_info->SensorHightSampling = 0;  /* 0 is default 1x */
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.cap.mipi_data_lp2hs_settle_dc;
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
	break;
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}
	return ERROR_NONE;
}

static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		preview(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		capture(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		normal_video(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		hs_video(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		slim_video(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		Custom1(image_window, sensor_config_data); // Custom1
		break;
	default:
		LOG_INF("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}

static kal_uint32 set_video_mode(UINT16 framerate)
{	/*This Function not used after ROME*/
	LOG_INF("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate */
	/***********
	 *if (framerate == 0)	 //Dynamic frame rate
	 *	return ERROR_NONE;
	 *spin_lock(&imgsensor_drv_lock);
	 *if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
	 *	imgsensor.current_fps = 296;
	 *else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
	 *	imgsensor.current_fps = 146;
	 *else
	 *	imgsensor.current_fps = framerate;
	 *spin_unlock(&imgsensor_drv_lock);
	 *set_max_framerate(imgsensor.current_fps, 1);
	 ********/
	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) /* enable auto flicker */
	    imgsensor.autoflicker_en = KAL_TRUE;
	else /* Cancel Auto flick */
	    imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ?
			(frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
		    set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length = imgsensor_info.normal_video.pclk / framerate * 10 /
			imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ?
			(frame_length - imgsensor_info.normal_video.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
		    set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
			frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ?
				(frame_length - imgsensor_info.cap1.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
		} else {
			if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
				LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
					framerate, imgsensor_info.cap.max_framerate / 10);
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ?
				(frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
		}
        if (imgsensor.frame_length > imgsensor.shutter)
		    set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ?
			(frame_length - imgsensor_info.hs_video.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
		    set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ?
			(frame_length - imgsensor_info.slim_video.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
		    set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ?
			(frame_length - imgsensor_info.custom1.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
		    set_dummy();
		break;
	default:  /* coding with  preview scenario by default */
		frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ?
			(frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
		    set_dummy();
		LOG_INF("error scenario_id = %d, we use preview scenario\n", scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		*framerate = imgsensor_info.pre.max_framerate;
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		*framerate = imgsensor_info.normal_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		*framerate = imgsensor_info.cap.max_framerate;
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		*framerate = imgsensor_info.hs_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		*framerate = imgsensor_info.slim_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		*framerate = imgsensor_info.custom1.max_framerate;
	break;
	default:
		break;
	}

	return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	pr_debug("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	if (enable){
		write_cmos_sensor(0xfe, 0x00);
		write_cmos_sensor(0xef, 0x90); /* stream on */
		write_cmos_sensor(0xfe, 0x00);
	} else {
		write_cmos_sensor(0xfe, 0x00);
		write_cmos_sensor(0xef, 0x00); /* stream off */
		write_cmos_sensor(0xfe, 0x00);
	}

	mdelay(10);
	return ERROR_NONE;
}

static kal_uint32 get_sensor_temperature(void)
{
    INT32 temperature_convert = 25;
    return temperature_convert;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
	UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *)feature_para;
	UINT16 *feature_data_16 = (UINT16 *)feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *)feature_para;
	UINT32 *feature_data_32 = (UINT32 *)feature_para;
	INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;
	/* unsigned long long *feature_return_para=(unsigned long long *) feature_para; */

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data = (MSDK_SENSOR_REG_INFO_STRUCT *)feature_para;

	LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = imgsensor.line_length;
		*feature_return_para_16 = imgsensor.frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*feature_return_para_32 = imgsensor.pclk;
		*feature_para_len = 4;
		break;
	/*********
	 *case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
	 *	{
	 *		kal_uint32 rate;
	 *
	 *		switch (*feature_data) {
	 *		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	 *			rate = imgsensor_info.cap.mipi_pixel_rate;
	 *			break;
	 *		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
	 *			rate = imgsensor_info.normal_video.mipi_pixel_rate;
	 *			break;
	 *		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
	 *			rate = imgsensor_info.hs_video.mipi_pixel_rate;
	 *			break;
	 *		case MSDK_SCENARIO_ID_SLIM_VIDEO:
	 *			rate = imgsensor_info.slim_video.mipi_pixel_rate;
	 *			break;
	 *		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	 *		default:
	 *			rate = imgsensor_info.pre.mipi_pixel_rate;
	 *			break;
	 *		}
	 *		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
	 *	}
	 *	break;
	 *******/
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(*feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		night_mode((BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain((UINT16)*feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE */
		/* if EEPROM does not exist in camera module. */
		*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(*feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		get_imgsensor_id(feature_return_para_32);
		break;
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode((BOOL)*feature_data_16, *(feature_data_16 + 1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: /*for factory mode auto testing */
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_16;
		spin_unlock(&imgsensor_drv_lock);
        	LOG_INF("current fps :%d\n", imgsensor.current_fps);
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length(
		(UINT16) *feature_data, (UINT16) *(feature_data + 1));
		break;
	case SENSOR_FEATURE_SET_HDR:
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_en = (BOOL)*feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32)*feature_data);

		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data + 1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[1], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[2], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[3], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[4], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[5],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[0], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		/*LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data + 1), (UINT16)*(feature_data + 2));
		ihdr_write_shutter_gain((UINT16)*feature_data,
			(UINT16)*(feature_data + 1), (UINT16)*(feature_data + 2));*/
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_i32 = get_sensor_temperature();
		*feature_para_len = 4;
	break;
	default:
		break;
	}

	return ERROR_NONE;
}

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 GC2375H_CHENGXIANGTONG_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}
