
#ifndef COMPONENTS_EPD_INCLUDE_EPD_H_
#define COMPONENTS_EPD_INCLUDE_EPD_H_

#ifdef  __cplusplus
extern "C" {
#endif
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../../io_driver/include/spidriver.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include "rom/tjpgd.h"
#include <sys/stat.h>
#include <errno.h>


#define EPD_WIDTH 640
#define EPD_HEIGHT 384

#define EPD_BUFFER_SIZE ((EPD_WIDTH) * (EPD_HEIGHT) / 8)

// === Color names constants ===
#define EPD_BLACK 15
#define EPD_WHITE 0

#define SCK_Pin		GPIO_NUM_18
#define MOSI_Pin	GPIO_NUM_23
#define MISO_Pin	GPIO_NUM_19
#define DC_Pin		GPIO_NUM_17
#define BUSY_Pin	GPIO_NUM_34
#define RST_Pin		GPIO_NUM_16
#define CS_Pin		GPIO_NUM_5

#define isEPD_BUSY  gpio_get_level(BUSY_Pin)
#define EPD_BUSY_LEVEL 0

#define LANDSCAPE_0		1
#define LANDSCAPE_180	2

// === Embedded fonts constants ===
#define DEFAULT_FONT	0
#define DEJAVU18_FONT	1
#define DEJAVU24_FONT	2
#define UBUNTU16_FONT	3
#define COMIC24_FONT	4
#define MINYA24_FONT	5
#define TOONEY32_FONT	6
#define SMALL_FONT		7
#define FONT_7SEG		8
#define USER_FONT		9  // font will be read from file

// === Special coordinates constants ===
#define CENTER	-9003
#define RIGHT	-9004
#define BOTTOM	-9004

#define LASTX	7000
#define LASTY	8000

#define swapMethod(a, b) { int16_t t = a; a = b; b = t; }

extern uint8_t tft_SmallFont[];
extern uint8_t tft_DefaultFont[];
extern uint8_t tft_Dejavu18[];
extern uint8_t tft_Dejavu24[];
extern uint8_t tft_Ubuntu16[];
extern uint8_t tft_Comic24[];
extern uint8_t tft_minya24[];
extern uint8_t tft_tooney32[];

#define DEG_TO_RAD 0.01745329252
#define RAD_TO_DEG 57.295779513
#define deg_to_rad 0.01745329252 + 3.14159265359

static const uint16_t font_bcd[] = {
  0x200, // 0010 0000 0000  // -
  0x080, // 0000 1000 0000  // .
  0x06C, // 0100 0110 1100  // /, degree
  0x05f, // 0000 0101 1111, // 0
  0x005, // 0000 0000 0101, // 1
  0x076, // 0000 0111 0110, // 2
  0x075, // 0000 0111 0101, // 3
  0x02d, // 0000 0010 1101, // 4
  0x079, // 0000 0111 1001, // 5
  0x07b, // 0000 0111 1011, // 6
  0x045, // 0000 0100 0101, // 7
  0x07f, // 0000 0111 1111, // 8
  0x07d, // 0000 0111 1101  // 9
  0x900  // 1001 0000 0000  // :
};

typedef uint8_t color_t;

typedef struct {
      uint8_t charCode;
      int adjYOffset;
      int width;
      int height;
      int xOffset;
      int xDelta;
      uint16_t dataPtr;
} propFont;

typedef struct {
	uint8_t 	*font;
	uint8_t 	x_size;
	uint8_t 	y_size;
	uint8_t	    offset;
	uint16_t	numchars;
    uint16_t	size;
	uint8_t 	max_x_size;
    uint8_t     bitmap;
	color_t     color;
} Font_t;

typedef struct {
	uint16_t        x1;
	uint16_t        y1;
	uint16_t        x2;
	uint16_t        y2;
} dispWin_t;

typedef struct {
	FILE		*fhndl;			// File handler for input function
    int			x;				// image top left point X position
    int			y;				// image top left point Y position
    uint8_t		*membuff;		// memory buffer containing the image
    uint32_t	bufsize;		// size of the memory buffer
    uint32_t	bufptr;
    dispWin_t *dispWin;	// memory buffer current position
    int width;
	int height;
	uint8_t *drawBuff;
} JPGIODEV;


typedef struct {
    const unsigned char *inData;
    int inPos;
    unsigned char *outData;
    int outW;
    int outH;
} JpegDev;

#define JPG_IMAGE_LINE_BUF_SIZE 512

void EPD_HW_Init();

void EPD_FillScreen(uint16_t color);
void EPD_Update();
void EPD_send8pixel(uint8_t data);
void EPD_wakeUp();
void EPD_sleep();
void EPD_sendCommand(uint8_t value);
void EPD_waitBusy();
void EPD_drawPixel(int x, int y, uint8_t val);
void EPD_DrawFastVLine(int16_t x, int16_t y, int16_t h, color_t color);
void EPD_pushColorRep(int x1, int y1, int x2, int y2, color_t color);
void EPD_SetFont(uint8_t font, const char *font_file);
void EPD_getMaxWidthHeight();
void EPD_DrawFastHLine(int16_t x, int16_t y, int16_t w, color_t color);
void EPD_DrawText(char *st, int x, int y);
int EPD_getStringWidth(char* str);
void EPD_draw7seg(int16_t x, int16_t y, int8_t num, int16_t w, int16_t l, color_t color);
int EPD_7seg_height();
int EPD_7seg_width();
void EPD_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color);
uint8_t EPD_getCharPtr(uint8_t c);
int EPD_rotatePropChar(int x, int y, int offset);
int EPD_printProportionalChar(int x, int y);
void EPD_DrawRect(uint16_t x1,uint16_t y1,uint16_t w,uint16_t h, color_t color);
void EPD_BarHor(int16_t x, int16_t y, int16_t w, int16_t l, color_t color, color_t outline);
void EPD_BarVert(int16_t x, int16_t y, int16_t w, int16_t l, color_t color, color_t outline);
void EPD_FillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color);
void EPD_rotateChar(uint8_t c, int x, int y, int pos);
void EPD_printChar(uint8_t c, int x, int y);
void EPD_DrawImageJpg(int x, int y, uint8_t scale, char *fname, uint8_t *buf, int size);

#ifdef  __cplusplus
}
#endif
#endif
