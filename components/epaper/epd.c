#include "epd.h"

static const char* TAG = "EPD";

static uint8_t 	drawBuff[EPD_BUFFER_SIZE];
int8_t 		_current_page = -1;
uint8_t   	orientation;
Font_t 		cfont;
color_t   	_fg = EPD_BLACK;            	// current foreground color for fonts
color_t   	_bg = EPD_WHITE;
uint16_t  font_rotate;   	// current font font_rotate angle (0~395)
uint8_t   font_transparent;	// if not 0 draw fonts transparent
uint8_t   font_forceFixed;  // if not zero force drawing proportional fonts with fixed width
uint8_t   font_buffered_char;
uint8_t   font_line_space;	// additional spacing between text lines; added to font height
uint8_t   text_wrap;        // if not 0 wrap long text to the new line, else clip
int	EPD_X;					// X position of the next character after EPD_print() function
int	EPD_Y;					// Y position of the next character after EPD_print() function
int EPD_OFFSET = 0;
propFont fontChar;
int image_debug = 1;


dispWin_t dispWin = {
	  .x1 = 0,
	  .y1 = 0,
	  .x2 = EPD_WIDTH-1,
	  .y2 = EPD_HEIGHT-1,
	};

void EPD_HW_Init()
{
	ESP_LOGI(TAG, ">> Init");

	gpio_set_direction(BUSY_Pin, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BUSY_Pin, GPIO_PULLUP_ONLY);
	gpio_set_direction(DC_Pin, GPIO_MODE_OUTPUT);
	gpio_set_direction(CS_Pin, GPIO_MODE_OUTPUT);

	ESP_LOGI(TAG, ">> Init Complete");
}

// User defined call-back function to input JPEG data from file
//---------------------
static UINT tjd_input (
	JDEC* jd,		// Decompression object
	BYTE* buff,		// Pointer to the read buffer (NULL:skip)
	UINT nd			// Number of bytes to read/skip from input stream
)
{
	int rb = 0;
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;

	if (buff) {	// Read nd bytes from the input strem
		rb = fread(buff, 1, nd, dev->fhndl);
		return rb;	// Returns actual number of bytes read
	}
	else {	// Remove nd bytes from the input stream
		if (fseek(dev->fhndl, nd, SEEK_CUR) >= 0) return nd;
		else return 0;
	}
}

// User defined call-back function to input JPEG data from memory buffer
//-------------------------
static UINT tjd_buf_input (
	JDEC* jd,		// Decompression object
	BYTE* buff,		// Pointer to the read buffer (NULL:skip)
	UINT nd			// Number of bytes to read/skip from input stream
)
{
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;
	if (!dev->membuff) return 0;
	if (dev->bufptr >= (dev->bufsize + 2)) return 0; // end of stream

	if ((dev->bufptr + nd) > (dev->bufsize + 2)) nd = (dev->bufsize + 2) - dev->bufptr;

	if (buff) {	// Read nd bytes from the input strem
		memcpy(buff, dev->membuff + dev->bufptr, nd);
		dev->bufptr += nd;
		return nd;	// Returns number of bytes read
	}
	else {	// Remove nd bytes from the input stream
		dev->bufptr += nd;
		return nd;
	}
}

// User defined call-back function to output RGB bitmap to display device
//----------------------
static UINT tjd_output (
	JDEC* jd,		// Decompression object of current session
	void* bitmap,	// Bitmap data to be output
	JRECT* rect		// Rectangular region to output
)
{
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;

	// ** Put the rectangular into the display device **
	uint8_t lvl_buf[16] = {32,70,110,150,185,210,220,225,230,235,240,243,248,251,253,255};
	uint8_t lvl_buf_jpg[16] = {4,8,12,16,22,30,40,60,80,110,140,180,220,240,250,255};
	int x;
	int y;
	int dleft, dtop, dright, dbottom;
	BYTE *src = (BYTE*)bitmap;

	int left = rect->left + dev->x;
	int top = rect->top + dev->y;
	int right = rect->right + dev->x;
	int bottom = rect->bottom + dev->y;

	if ((left > dev->dispWin->x2) || (top > dev->dispWin->y2)) return 1;	// out of screen area, return
	if ((right < dev->dispWin->x1) || (bottom < dev->dispWin->y1)) return 1;// out of screen area, return

	if (left < dev->dispWin->x1) dleft = dev->dispWin->x1;
	else dleft = left;
	if (top < dev->dispWin->y1) dtop = dev->dispWin->y1;
	else dtop = top;
	if (right > dev->dispWin->x2) dright = dev->dispWin->x2;
	else dright = right;
	if (bottom > dev->dispWin->y2) dbottom = dev->dispWin->y2;
	else dbottom = bottom;

	if ((dleft > dev->dispWin->x2) || (dtop > dev->dispWin->y2)) return 1;		// out of screen area, return
	if ((dright < dev->dispWin->x1) || (dbottom < dev->dispWin->y1)) return 1;	// out of screen area, return

	uint32_t len = ((dright-dleft+1) * (dbottom-dtop+1));	// calculate length of data

	int gs_clr = 0;
	uint8_t rgb_color[3];
	uint8_t last_lvl;
	uint16_t i;
	uint8_t pix;
	if ((len > 0) && (len <= JPG_IMAGE_LINE_BUF_SIZE)) {
		for (y = top; y <= bottom; y++) {
			for (x = left; x <= right; x++) {
				memcpy(rgb_color, src, 3);
				src += 3;
				gs_clr = rgb_color[0] + rgb_color[1] + rgb_color[2];
				i = x / 8 + y * dev->width / 8;
				if(gs_clr < 400) {
					drawBuff[i] = (drawBuff[i] | (1 << (7 - x % 8)));
				}else{
					drawBuff[i] = (drawBuff[i] & (0xFF ^ (1 << (7 - x % 8))));
				}
			}
		}
	}
	else {
		printf("Data size error: %d jpg: (%d,%d,%d,%d) disp: (%d,%d,%d,%d)\r\n", len, left,top,right,bottom, dleft,dtop,dright,dbottom);
		return 0;  // stop decompression
	}

	return 1;	// Continue to decompression
}

void EPD_DrawImageJpg(int x, int y, uint8_t scale, char *fname, uint8_t *buf, int size)
{
	JPGIODEV dev;
	struct stat sb;
	char *work = NULL;		// Pointer to the working buffer (must be 4-byte aligned)
	UINT sz_work = 3800;	// Size of the working buffer (must be power of 2)
	JDEC jd;				// Decompression object (70 bytes)
	JRESULT rc;
	int res = -10;

	dev.dispWin = &dispWin;
	dev.width = EPD_WIDTH;
	dev.height = EPD_HEIGHT;
	dev.drawBuff = drawBuff;

	if (fname == NULL) {
		// image from buffer
		dev.fhndl = NULL;
		dev.membuff = buf;
		dev.bufsize = size;
		dev.bufptr = 0;
	}
	else {
		// image from file
		dev.membuff = NULL;
		dev.bufsize = 0;
		dev.bufptr = 0;

		if (stat(fname, &sb) != 0) {
			if (image_debug) printf("File error: %ss\r\n", strerror(errno));
			res = -11;
		}

		dev.fhndl = fopen(fname, "r");
		if (!dev.fhndl) {
			if (image_debug) printf("Error opening file: %s\r\n", strerror(errno));
			res = -12;
		}
	}

	if (scale > 3) scale = 3;

	work = (char*)malloc(sz_work);
	if (work) {
		if (dev.membuff) rc = jd_prepare(&jd, tjd_buf_input, (void *)work, sz_work, &dev);
		else rc = jd_prepare(&jd, tjd_input, (void *)work, sz_work, &dev);
		if (rc == JDR_OK) {
			if (x == CENTER) x = ((dispWin.x2 - dispWin.x1 + 1 - (int)(jd.width >> scale)) / 2) + dispWin.x1;
			else if (x == RIGHT) x = dispWin.x2 + 1 - (int)(jd.width >> scale);

			if (y == CENTER) y = ((dispWin.y2 - dispWin.y1 + 1 - (int)(jd.height >> scale)) / 2) + dispWin.y1;
			else if (y == BOTTOM) y = dispWin.y2 + 1 - (int)(jd.height >> scale);

			if (x < ((dispWin.x2-1) * -1)) x = (dispWin.x2-1) * -1;
			if (y < ((dispWin.y2-1)) * -1) y = (dispWin.y2-1) * -1;
			if (x > (dispWin.x2-1)) x = dispWin.x2 - 1;
			if (y > (dispWin.y2-1)) y = dispWin.y2-1;

			dev.x = x;
			dev.y = y;

			// Start to decode the JPEG file
			rc = jd_decomp(&jd, tjd_output, scale);

			if (rc != JDR_OK) {
				if (image_debug) printf("jpg decompression error %d\r\n", rc);
				res = rc * -1;
			}
			res = 0;
			if (image_debug) printf("Jpg size: %dx%d, position; %d,%d, scale: %d, bytes used: %d\r\n", jd.width, jd.height, x, y, scale, jd.sz_pool);
		}
		else {
			if (image_debug) printf("jpg prepare error %d\r\n", rc);
			res = rc * -1;
		}
	}
	else {
		if (image_debug) printf("work buffer allocation error\r\n");
		res = -13;
	}
	if (work) free(work);  // free work buffer
	if (dev.fhndl) fclose(dev.fhndl);  // close input file
}

void EPD_FillScreen(uint16_t color) {
	uint8_t data = (color == EPD_BLACK) ? 0xFF : 0x00;
	for (uint16_t x = 0; x < sizeof(drawBuff); x++)
	{
		drawBuff[x] = data;
	}
}

void EPD_Update() {

	ESP_LOGD(TAG, ">> Update Begin");

	EPD_wakeUp();
	EPD_sendCommand(0x10);

	for (uint32_t i = 0; i < EPD_BUFFER_SIZE; i++)
	{
		EPD_send8pixel(i < sizeof(drawBuff) ? drawBuff[i] : 0x00);
	}
	EPD_sendCommand(0x12);
	EPD_waitBusy();
	EPD_sleep();

	ESP_LOGD(TAG, ">> Update End");

}

void EPD_SetFont(uint8_t font, const char *font_file)
{
  cfont.font = NULL;

  if (font == FONT_7SEG) {
    cfont.bitmap = 2;
    cfont.x_size = 24;
    cfont.y_size = 6;
    cfont.offset = 0;
    cfont.color  = _fg;
  }
  else {
	  if (font == DEJAVU18_FONT) cfont.font = tft_Dejavu18;
	  else if (font == DEJAVU24_FONT) cfont.font = tft_Dejavu24;
	  else if (font == UBUNTU16_FONT) cfont.font = tft_Ubuntu16;
	  else if (font == COMIC24_FONT) cfont.font = tft_Comic24;
	  else if (font == MINYA24_FONT) cfont.font = tft_minya24;
	  else if (font == TOONEY32_FONT) cfont.font = tft_tooney32;
	  else if (font == SMALL_FONT) cfont.font = tft_SmallFont;
	  else cfont.font = tft_DefaultFont;

	  cfont.bitmap = 1;
	  cfont.x_size = cfont.font[0];
	  cfont.y_size = cfont.font[1];
	  if (cfont.x_size > 0) {
		  cfont.offset = cfont.font[2];
		  cfont.numchars = cfont.font[3];
		  cfont.size = cfont.x_size * cfont.y_size * cfont.numchars;
	  }
	  else {
		  cfont.offset = 4;
		  EPD_getMaxWidthHeight();
	  }
	  //_testFont();
  }
}

void EPD_getMaxWidthHeight()
{
	uint16_t tempPtr = 4; // point at first char data
	uint8_t cc, cw, ch, cd, cy;

	cfont.numchars = 0;
	cfont.max_x_size = 0;

    cc = cfont.font[tempPtr++];
    while (cc != 0xFF)  {
    	cfont.numchars++;
        cy = cfont.font[tempPtr++];
        cw = cfont.font[tempPtr++];
        ch = cfont.font[tempPtr++];
        tempPtr++;
        cd = cfont.font[tempPtr++];
        cy += ch;
		if (cw > cfont.max_x_size) cfont.max_x_size = cw;
		if (cd > cfont.max_x_size) cfont.max_x_size = cd;
		if (ch > cfont.y_size) cfont.y_size = ch;
		if (cy > cfont.y_size) cfont.y_size = cy;
		if (cw != 0) {
			// packed bits
			tempPtr += (((cw * ch)-1) / 8) + 1;
		}
	    cc = cfont.font[tempPtr++];
	}
    cfont.size = tempPtr;
}

void EPD_pushColorRep(int x1, int y1, int x2, int y2, color_t color)
{
	color &= 0x0F;
	for (int y=y1; y<=y2; y++) {
		for (int x = x1; x<=x2; x++){
			EPD_drawPixel(x, y, color);
		}
	}

}

void EPD_DrawFastVLine(int16_t x, int16_t y, int16_t h, color_t color) {
	// clipping
	if ((x < 0) || (x > EPD_WIDTH) || (y > EPD_HEIGHT)) return;
	EPD_pushColorRep(x, y, x, y+h-1, color);
}

void EPD_DrawFastHLine(int16_t x, int16_t y, int16_t w, color_t color) {
	// clipping
	if ((x < 0) || (x > EPD_WIDTH) || (y > EPD_HEIGHT)) return;
	EPD_pushColorRep(x, y, x+w-1, y, color);
}

void EPD_send8pixel(uint8_t data)
{
  for (uint8_t j = 0; j < 8; j++)
  {
    uint8_t t = data & 0x80 ? 0x00 : 0x03;
    t <<= 4;
    data <<= 1;
    j++;
    t |= data & 0x80 ? 0x00 : 0x03;
    data <<= 1;
    SPI_TransferByte(t);
  }
}

void EPD_DrawText(char *st, int x, int y) {
	int stl, i, tmpw, tmph, fh;
	uint8_t ch;

	if (cfont.bitmap == 0) return; // wrong font selected

	// ** Rotated strings cannot be aligned
	if ((font_rotate != 0) && ((x <= CENTER) || (y <= CENTER))) return;

	if ((x < LASTX) || (font_rotate == 0)) EPD_OFFSET = 0;

	if ((x >= LASTX) && (x < LASTY)) x = EPD_X + (x-LASTX);
	else if (x > CENTER) x += 0;

	if (y >= LASTY) y = EPD_Y + (y-LASTY);
	else if (y > CENTER) y += 0;

	// ** Get number of characters in string to print
	stl = strlen(st);

	// ** Calculate CENTER, RIGHT or BOTTOM position
	tmpw = EPD_getStringWidth(st);	// string width in pixels
	fh = cfont.y_size;			// font height
	if ((cfont.x_size != 0) && (cfont.bitmap == 2)) {
		// 7-segment font
		fh = (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size);  // 7-seg character height
	}


	 EPD_X = x;
	 EPD_Y = y;

	// ** Adjust y position
	tmph = cfont.y_size; // font height
	// for non-proportional fonts, char width is the same for all chars
	tmpw = cfont.x_size;
	if (cfont.x_size != 0) {
		if (cfont.bitmap == 2) {	// 7-segment font
			tmpw = EPD_7seg_width();	// character width
			tmph = EPD_7seg_height();	// character height
		}
	}
	else EPD_OFFSET = 0;	// fixed font; offset not needed

	if (( EPD_Y + tmph - 1) > EPD_HEIGHT) return;

	int offset = EPD_OFFSET;

	for (i=0; i<stl; i++) {
		ch = st[i]; // get string character

		if (ch == 0x0D) { // === '\r', erase to eol ====
			if ((!font_transparent) && (font_rotate==0)) EPD_FillRect( EPD_X, EPD_Y,  EPD_WIDTH+1- EPD_X, tmph, _bg);
		}

		else if (ch == 0x0A) { // ==== '\n', new line ====
			if (cfont.bitmap == 1) {
				 EPD_Y += tmph + font_line_space;
				if ( EPD_Y > (EPD_WIDTH-tmph)) break;
				 EPD_X = 0;
			}
		}

		else { // ==== other characters ====
			if (cfont.x_size == 0) {
				// for proportional font get character data to 'fontChar'
				if (EPD_getCharPtr(ch)) tmpw = fontChar.xDelta;
				else continue;
			}

			// check if character can be displayed in the current line
			if (( EPD_X+tmpw) > (EPD_WIDTH)) {
				if (text_wrap == 0) break;
				 EPD_Y += tmph + font_line_space;
				if ( EPD_Y > (EPD_HEIGHT-tmph)) break;
				 EPD_X = 0;
			}

			// Let's print the character
			if (cfont.x_size == 0) {
				// == proportional font
				if (font_rotate == 0) EPD_X += EPD_printProportionalChar( EPD_X, EPD_Y) + 1;
				else {
					// rotated proportional font
					offset += EPD_rotatePropChar(x, y, offset);
					EPD_OFFSET = offset;
				}
			}
			else {
				if (cfont.bitmap == 1) {
					// == fixed font
					if ((ch < cfont.offset) || ((ch-cfont.offset) > cfont.numchars)) ch = cfont.offset;
					if (font_rotate == 0) {
						EPD_printChar(ch, EPD_X, EPD_Y);
						 EPD_X += tmpw;
					}
					else EPD_rotateChar(ch, x, y, i);
				}
				else if (cfont.bitmap == 2) {
					// == 7-segment font ==
					EPD_draw7seg( EPD_X, EPD_Y, ch, cfont.y_size, cfont.x_size, _fg);
					 EPD_X += (tmpw + 2);
				}
			}
		}
	}
}

void EPD_printChar(uint8_t c, int x, int y) {
	uint8_t i, j, ch, fz, mask;
	uint16_t k, temp, cx, cy;

	// fz = bytes per char row
	fz = cfont.x_size/8;
	if (cfont.x_size % 8) fz++;

	// get character position in buffer
	temp = ((c-cfont.offset)*((fz)*cfont.y_size))+4;

	if (!font_transparent) EPD_FillRect(x, y, cfont.x_size, cfont.y_size, _bg);

	for (j=0; j<cfont.y_size; j++) {
		for (k=0; k < fz; k++) {
			ch = cfont.font[temp+k];
			mask=0x80;
			for (i=0; i<8; i++) {
				if ((ch & mask) !=0) {
					cx = (uint16_t)(x+i+(k*8));
					cy = (uint16_t)(y+j);
					EPD_drawPixel(cx, cy, _fg);
				}
				mask >>= 1;
			}
		}
		temp += (fz);
	}
}

void EPD_rotateChar(uint8_t c, int x, int y, int pos) {
  uint8_t i,j,ch,fz,mask;
  uint16_t temp;
  int newx,newy;
  double radian = font_rotate*0.0175;
  float cos_radian = cos(radian);
  float sin_radian = sin(radian);
  int zz;

  if( cfont.x_size < 8 ) fz = cfont.x_size;
  else fz = cfont.x_size/8;
  temp=((c-cfont.offset)*((fz)*cfont.y_size))+4;

  for (j=0; j<cfont.y_size; j++) {
    for (zz=0; zz<(fz); zz++) {
      ch = cfont.font[temp+zz];
      mask = 0x80;
      for (i=0; i<8; i++) {
        newx=(int)(x+(((i+(zz*8)+(pos*cfont.x_size))*cos_radian)-((j)*sin_radian)));
        newy=(int)(y+(((j)*cos_radian)+((i+(zz*8)+(pos*cfont.x_size))*sin_radian)));

        if ((ch & mask) != 0) EPD_drawPixel(newx,newy,_fg);
        else if (!font_transparent) EPD_drawPixel(newx,newy,_bg);
        mask >>= 1;
      }
    }
    temp+=(fz);
  }
  // calculate x,y for the next char
  EPD_X = (int)(x + ((pos+1) * cfont.x_size * cos_radian));
  EPD_Y = (int)(y + ((pos+1) * cfont.x_size * sin_radian));
}

void EPD_draw7seg(int16_t x, int16_t y, int8_t num, int16_t w, int16_t l, color_t color) {
  /* TODO: clipping */
  if (num < 0x2D || num > 0x3A) return;

  int16_t c = font_bcd[num-0x2D];
  int16_t d = 2*w+l+1;

  // === Clear unused segments ===
  /*
  if (!(c & 0x001)) barVert(x+d, y+d, w, l, _bg, _bg);
  if (!(c & 0x002)) barVert(x,   y+d, w, l, _bg, _bg);
  if (!(c & 0x004)) barVert(x+d, y, w, l, _bg, _bg);
  if (!(c & 0x008)) barVert(x,   y, w, l, _bg, _bg);
  if (!(c & 0x010)) barHor(x, y+2*d, w, l, _bg, _bg);
  if (!(c & 0x020)) barHor(x, y+d, w, l, _bg, _bg);
  if (!(c & 0x040)) barHor(x, y, w, l, _bg, _bg);

  if (!(c & 0x080)) {
    // low point
    _fillRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, _bg);
    if (cfont.offset) _drawRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, _bg);
  }
  if (!(c & 0x100)) {
    // down middle point
    _fillRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, _bg);
    if (cfont.offset) _drawRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, _bg);
  }
  if (!(c & 0x800)) {
	// up middle point
    _fillRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, _bg);
    if (cfont.offset) _drawRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, _bg);
  }
  if (!(c & 0x200)) {
    // middle, minus
    _fillRect(x+2*w+1, y+d, l, 2*w+1, _bg);
    if (cfont.offset) _drawRect(x+2*w+1, y+d, l, 2*w+1, _bg);
  }
  */
  EPD_BarVert(x+d, y+d, w, l, _bg, _bg);
  EPD_BarVert(x,   y+d, w, l, _bg, _bg);
  EPD_BarVert(x+d, y, w, l, _bg, _bg);
  EPD_BarVert(x,   y, w, l, _bg, _bg);
  EPD_BarHor(x, y+2*d, w, l, _bg, _bg);
  EPD_BarHor(x, y+d, w, l, _bg, _bg);
  EPD_BarHor(x, y, w, l, _bg, _bg);

  EPD_FillRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, _bg);
  EPD_DrawRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, _bg);
  EPD_FillRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, _bg);
  EPD_DrawRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, _bg);
  EPD_FillRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, _bg);
  EPD_DrawRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, _bg);
  EPD_FillRect(x+2*w+1, y+d, l, 2*w+1, _bg);
  EPD_DrawRect(x+2*w+1, y+d, l, 2*w+1, _bg);

  // === Draw used segments ===
  if (c & 0x001) EPD_BarVert(x+d, y+d, w, l, color, cfont.color);	// down right
  if (c & 0x002) EPD_BarVert(x,   y+d, w, l, color, cfont.color);	// down left
  if (c & 0x004) EPD_BarVert(x+d, y, w, l, color, cfont.color);		// up right
  if (c & 0x008) EPD_BarVert(x,   y, w, l, color, cfont.color);		// up left
  if (c & 0x010) EPD_BarHor(x, y+2*d, w, l, color, cfont.color);	// down
  if (c & 0x020) EPD_BarHor(x, y+d, w, l, color, cfont.color);		// middle
  if (c & 0x040) EPD_BarHor(x, y, w, l, color, cfont.color);		// up

  if (c & 0x080) {
    // low point
	EPD_FillRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, color);
    if (cfont.offset) EPD_DrawRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, cfont.color);
  }
  if (c & 0x100) {
    // down middle point
	EPD_FillRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, color);
    if (cfont.offset) EPD_DrawRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, cfont.color);
  }
  if (c & 0x800) {
	// up middle point
    EPD_FillRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, color);
    if (cfont.offset) EPD_DrawRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, cfont.color);
  }
  if (c & 0x200) {
    // middle, minus
	EPD_FillRect(x+2*w+1, y+d, l, 2*w+1, color);
    if (cfont.offset) EPD_DrawRect(x+2*w+1, y+d, l, 2*w+1, cfont.color);
  }
}

void EPD_BarVert(int16_t x, int16_t y, int16_t w, int16_t l, color_t color, color_t outline) {
  EPD_FillTriangle(x+1, y+2*w, x+w, y+w+1, x+2*w-1, y+2*w, color);
  EPD_FillTriangle(x+1, y+2*w+l+1, x+w, y+3*w+l, x+2*w-1, y+2*w+l+1, color);
  EPD_DrawRect(x, y+2*w+1, 2*w+1, l, color);
  if (cfont.offset) {
	  EPD_FillTriangle(x+1, y+2*w, x+w, y+w+1, x+2*w-1, y+2*w, outline);
    EPD_FillTriangle(x+1, y+2*w+l+1, x+w, y+3*w+l, x+2*w-1, y+2*w+l+1, outline);
    EPD_DrawRect(x, y+2*w+1, 2*w+1, l, outline);
  }
}

void EPD_BarHor(int16_t x, int16_t y, int16_t w, int16_t l, color_t color, color_t outline) {
  EPD_FillTriangle(x+2*w, y+2*w-1, x+w+1, y+w, x+2*w, y+1, color);
  EPD_FillTriangle(x+2*w+l+1, y+2*w-1, x+3*w+l, y+w, x+2*w+l+1, y+1, color);
  EPD_DrawRect(x+2*w+1, y, l, 2*w+1, color);
  if (cfont.offset) {
	  EPD_FillTriangle(x+2*w, y+2*w-1, x+w+1, y+w, x+2*w, y+1, outline);
    EPD_FillTriangle(x+2*w+l+1, y+2*w-1, x+3*w+l, y+w, x+2*w+l+1, y+1, outline);
    EPD_DrawRect(x+2*w+1, y, l, 2*w+1, outline);
  }
}

void EPD_FillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color)
{
  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
	  swapMethod(y0, y1); swapMethod(x0, x1);
  }
  if (y1 > y2) {
	  swapMethod(y2, y1); swapMethod(x2, x1);
  }
  if (y0 > y1) {
	  swapMethod(y0, y1); swapMethod(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    EPD_DrawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  int32_t
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swapMethod(a,b);
    EPD_DrawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swapMethod(a,b);
    EPD_DrawFastHLine(a, y, b-a+1, color);
  }
}

void EPD_DrawRect(uint16_t x1,uint16_t y1,uint16_t w,uint16_t h, color_t color) {
  EPD_DrawFastHLine(x1,y1,w, color);
  EPD_DrawFastVLine(x1+w-1,y1,h, color);
  EPD_DrawFastHLine(x1,y1+h-1,w, color);
  EPD_DrawFastVLine(x1,y1,h, color);
}

int EPD_getStringWidth(char* str)
{
    int strWidth = 0;

	if (cfont.bitmap == 2) strWidth = ((EPD_7seg_width()+2) * strlen(str)) - 2;	// 7-segment font
	else if (cfont.x_size != 0) strWidth = strlen(str) * cfont.x_size;			// fixed width font
	else {
		// calculate the width of the string of proportional characters
		char* tempStrptr = str;
		while (*tempStrptr != 0) {
			if (EPD_getCharPtr(*tempStrptr++)) {
				strWidth += (((fontChar.width > fontChar.xDelta) ? fontChar.width : fontChar.xDelta) + 1);
			}
		}
		strWidth--;
	}
	return strWidth;
}

int EPD_7seg_width()
{
	return (2 * (2 * cfont.y_size + 1)) + cfont.x_size;
}

void EPD_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color) {
	if ((x >= EPD_WIDTH) || (y > EPD_HEIGHT)) return;

	if (w < 0) w = 0;
	if (h < 0) h = 0;

	if ((x + w) > (EPD_WIDTH+1)) w = EPD_WIDTH - x + 1;
	if ((y + h) > (EPD_HEIGHT+1)) h = EPD_HEIGHT - y + 1;
	if (w == 0) w = 1;
	if (h == 0) h = 1;
	EPD_pushColorRep(x, y, x+w-1, y+h-1, color);
}

int EPD_7seg_height()
{
	return (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size);
}

uint8_t EPD_getCharPtr(uint8_t c) {
  uint16_t tempPtr = 4; // point at first char data
  do {
	fontChar.charCode = cfont.font[tempPtr++];
    if (fontChar.charCode == 0xFF) return 0;

    fontChar.adjYOffset = cfont.font[tempPtr++];
    fontChar.width = cfont.font[tempPtr++];
    fontChar.height = cfont.font[tempPtr++];
    fontChar.xOffset = cfont.font[tempPtr++];
    fontChar.xOffset = fontChar.xOffset < 0x80 ? fontChar.xOffset : -(0xFF - fontChar.xOffset);
    fontChar.xDelta = cfont.font[tempPtr++];

    if (c != fontChar.charCode && fontChar.charCode != 0xFF) {
      if (fontChar.width != 0) {
        // packed bits
        tempPtr += (((fontChar.width * fontChar.height)-1) / 8) + 1;
      }
    }
  } while ((c != fontChar.charCode) && (fontChar.charCode != 0xFF));

  fontChar.dataPtr = tempPtr;
  if (c == fontChar.charCode) {
    if (font_forceFixed > 0) {
      // fix width & offset for forced fixed width
      fontChar.xDelta = cfont.max_x_size;
      fontChar.xOffset = (fontChar.xDelta - fontChar.width) / 2;
    }
  }
  else return 0;

  return 1;
}

int EPD_printProportionalChar(int x, int y) {
	uint8_t ch = 0;
	int i, j, char_width;

	char_width = ((fontChar.width > fontChar.xDelta) ? fontChar.width : fontChar.xDelta);
	int cx, cy;
	if (!font_transparent) EPD_FillRect(x, y, char_width+1, cfont.y_size, _bg);
	// draw Glyph
	uint8_t mask = 0x80;
	for (j=0; j < fontChar.height; j++) {
		for (i=0; i < fontChar.width; i++) {
			if (((i + (j*fontChar.width)) % 8) == 0) {
				mask = 0x80;
				ch = cfont.font[fontChar.dataPtr++];
			}

			if ((ch & mask) !=0) {
				cx = (uint16_t)(x+fontChar.xOffset+i);
				cy = (uint16_t)(y+j+fontChar.adjYOffset);
				EPD_drawPixel(cx, cy, _fg);
			}
			mask >>= 1;
		}
	}

	return char_width;
}

int EPD_rotatePropChar(int x, int y, int offset) {
  uint8_t ch = 0;
  double radian = font_rotate * DEG_TO_RAD;
  float cos_radian = cos(radian);
  float sin_radian = sin(radian);

  uint8_t mask = 0x80;
  for (int j=0; j < fontChar.height; j++) {
    for (int i=0; i < fontChar.width; i++) {
      if (((i + (j*fontChar.width)) % 8) == 0) {
        mask = 0x80;
        ch = cfont.font[fontChar.dataPtr++];
      }

      int newX = (int)(x + (((offset + i) * cos_radian) - ((j+fontChar.adjYOffset)*sin_radian)));
      int newY = (int)(y + (((j+fontChar.adjYOffset) * cos_radian) + ((offset + i) * sin_radian)));

      if ((ch & mask) != 0) EPD_drawPixel(newX,newY,_fg);
      else if (!font_transparent) EPD_drawPixel(newX,newY,_bg);

      mask >>= 1;
    }
  }

  return fontChar.xDelta+1;
}

void EPD_wakeUp()
{
    /**********************************release flash sleep**********************************/
  EPD_sendCommand(0X65);     //FLASH CONTROL
  SPI_TransferByte(0x01);

  EPD_sendCommand(0xAB);

  EPD_sendCommand(0X65);     //FLASH CONTROL
  SPI_TransferByte(0x00);
  /**********************************release flash sleep**********************************/
  EPD_sendCommand(0x01);
  SPI_TransferByte (0x37);       //POWER SETTING
  SPI_TransferByte (0x00);

  EPD_sendCommand(0X00);     //PANNEL SETTING
  SPI_TransferByte(0xCF);
  SPI_TransferByte(0x08);

  EPD_sendCommand(0x06);     //boost
  SPI_TransferByte (0xc7);
  SPI_TransferByte (0xcc);
  SPI_TransferByte (0x28);

  EPD_sendCommand(0x30);     //PLL setting
  SPI_TransferByte (0x3c);

  EPD_sendCommand(0X41);     //TEMPERATURE SETTING
  SPI_TransferByte(0x00);

  EPD_sendCommand(0X50);     //VCOM AND DATA INTERVAL SETTING
  SPI_TransferByte(0x77);

  EPD_sendCommand(0X60);     //TCON SETTING
  SPI_TransferByte(0x22);

  EPD_sendCommand(0x61);     //tres 640*384
  SPI_TransferByte (0x02);       //source 640
  SPI_TransferByte (0x80);
  SPI_TransferByte (0x01);       //gate 384
  SPI_TransferByte (0x80);

  EPD_sendCommand(0X82);     //VDCS SETTING
  SPI_TransferByte(0x1E);        //decide by LUT file

  EPD_sendCommand(0xe5);     //FLASH MODE
  SPI_TransferByte(0x03);

  EPD_sendCommand(0x04);     //POWER ON
  EPD_waitBusy();
}

void EPD_sleep()
{
	EPD_sendCommand(0X65);     //FLASH CONTROL
	SPI_TransferByte(0x01);

	EPD_sendCommand(0xB9);

	EPD_sendCommand(0X65);     //FLASH CONTROL
	SPI_TransferByte(0x00);

	EPD_sendCommand(0x02);     // POWER OFF
	EPD_waitBusy();
}

void EPD_sendCommand(uint8_t value) {
	gpio_set_level(GPIO_NUM_17, 0);
	gpio_set_level(GPIO_NUM_5, 0);
	SPI_TransferByte(value);
	gpio_set_level(GPIO_NUM_5, 1);
	gpio_set_level(GPIO_NUM_17, 1);
}

void EPD_waitBusy()
{
	while(isEPD_BUSY == EPD_BUSY_LEVEL) {
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

void EPD_drawPixel(int x, int y, uint8_t val)
{
	if (orientation == LANDSCAPE_180) {
		x = EPD_WIDTH - x - 1;
		y = EPD_HEIGHT - y - 1;
	}

	uint16_t i = x / 8 + y * EPD_WIDTH / 8;
	if (_current_page < 1)
	{
		if (i >= sizeof(drawBuff)) return;
	}
	else
	{
		y -= _current_page * EPD_HEIGHT;
		if ((y < 0) || (y >= EPD_HEIGHT)) return;
		i = x / 8 + y * EPD_WIDTH / 8;
	}
	if (val)
	  drawBuff[i] = (drawBuff[i] | (1 << (7 - x % 8)));
	else
	  drawBuff[i] = (drawBuff[i] & (0xFF ^ (1 << (7 - x % 8))));

}
