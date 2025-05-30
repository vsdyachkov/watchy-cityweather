// status bar icons
static const unsigned char PROGMEM battery[] = {0x1c,0x00,0x1c,0x00,0x7f,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0xbe,0x80,0xbe,0x80,0xbe,0x80,0x80,0x80,0xbe,0x80,0xbe,0x80,0xbe,0x80,0x80,0x80,0x7f,0x00};
static const unsigned char PROGMEM wifi[] = {0x01,0xf0,0x00,0x07,0xfc,0x00,0x1e,0x0f,0x00,0x39,0xf3,0x80,0x77,0xfd,0xc0,0xef,0x1e,0xe0,0x5c,0xe7,0x40,0x3b,0xfb,0x80,0x17,0x1d,0x00,0x0e,0xee,0x00,0x05,0xf4,0x00,0x03,0xb8,0x00,0x01,0x50,0x00,0x00,0xe0,0x00,0x00,0x40,0x00,0x00,0x00,0x00};
static const unsigned char PROGMEM geolocation[] = {0x0f,0x80,0x3f,0xe0,0x7f,0xf0,0x7f,0xf0,0xf8,0xf8,0xf0,0x78,0xf0,0x78,0x70,0x70,0x78,0xf0,0x3f,0xe0,0x3f,0xe0,0x1f,0xc0,0x0f,0x80,0x07,0x00,0x07,0x00,0x02,0x00};
static const unsigned char PROGMEM ntp[] = {0x0f,0x00,0x1f,0xc0,0x32,0x60,0x60,0x30,0x42,0x10,0xc2,0x18,0xe2,0x38,0xc1,0x18,0x40,0x90,0x60,0x30,0x32,0x60,0x1f,0xc0,0x07,0x00,0xff,0xf8,0xff,0xf8,0x07,0x00};
static const unsigned char PROGMEM weather[] = {0x00,0x00,0x01,0x00,0x07,0xc0,0x1f,0xf0,0x3f,0xf8,0x7f,0xfc,0xff,0xfe,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x05,0x00,0x02,0x00,0x00,0x00};

// weather icons

// 'sun', 25x25px
const unsigned char sun [] PROGMEM = {
	0x00, 0x08, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x0c, 0x1c, 0x18, 0x00, 0x0e, 0x1c, 0x38, 0x00, 
	0x07, 0x1c, 0x70, 0x00, 0x03, 0x00, 0x60, 0x00, 0x60, 0x3e, 0x03, 0x00, 0x71, 0xff, 0xc7, 0x00, 
	0x39, 0xe1, 0xce, 0x00, 0x19, 0x80, 0xcc, 0x00, 0x03, 0x80, 0x60, 0x00, 0x7b, 0x00, 0x6f, 0x00, 
	0xfb, 0x00, 0x6f, 0x80, 0x7b, 0x00, 0x6f, 0x00, 0x03, 0x00, 0x60, 0x00, 0x19, 0x80, 0xcc, 0x00, 
	0x39, 0xc1, 0xce, 0x00, 0x71, 0xff, 0xc7, 0x00, 0x60, 0x3e, 0x03, 0x00, 0x03, 0x00, 0x60, 0x00, 
	0x07, 0x1c, 0x70, 0x00, 0x0e, 0x1c, 0x38, 0x00, 0x0c, 0x1c, 0x18, 0x00, 0x00, 0x1c, 0x00, 0x00, 
	0x00, 0x08, 0x00, 0x00
};

// 'cloudSun', 25x25px
const unsigned char cloudSun [] PROGMEM = {
	0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x03, 0x03, 0x00, 0x03, 0x80, 0x07, 0x00, 
	0x01, 0xc7, 0x8e, 0x00, 0x00, 0xcf, 0xcc, 0x00, 0x00, 0x18, 0x60, 0x00, 0x00, 0x38, 0x70, 0x00, 
	0x0f, 0x60, 0x1b, 0x80, 0x0f, 0x60, 0x1b, 0x80, 0x00, 0x60, 0x18, 0x00, 0x01, 0xf8, 0x70, 0x00, 
	0x01, 0xfe, 0x60, 0x00, 0x03, 0x03, 0xc6, 0x00, 0x06, 0x01, 0x87, 0x00, 0x0c, 0x00, 0xc3, 0x00, 
	0x1c, 0x00, 0x60, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x60, 0x00, 0x3e, 0x00, 0xc0, 0x00, 0x03, 0x00, 
	0xc0, 0x00, 0x01, 0x80, 0xe0, 0x00, 0x01, 0x80, 0x70, 0x00, 0x01, 0x80, 0x3f, 0xff, 0xff, 0x00, 
	0x1f, 0xff, 0xfe, 0x00
};

// 'fog', 25x25px
const unsigned char fog [] PROGMEM = {
	0x00, 0xfc, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x06, 0x01, 0x80, 0x00, 
	0x0c, 0x00, 0xc0, 0x00, 0x1c, 0x00, 0x60, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x60, 0x00, 0x3e, 0x00, 
	0xc0, 0x00, 0x03, 0x00, 0xc0, 0x00, 0x01, 0x80, 0xe0, 0x00, 0x01, 0x80, 0x70, 0x00, 0x01, 0x80, 
	0x3f, 0xff, 0xff, 0x00, 0x1f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x3f, 0xfe, 0x7e, 0x00, 0x3f, 0xfe, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x9f, 0xfc, 0x00, 
	0x1f, 0x9f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xfc, 0xf8, 0x00, 0x0f, 0xfc, 0xf8, 0x00, 
	0x00, 0x00, 0x00, 0x00
};

// 'rain', 25x25px
const unsigned char rain [] PROGMEM = {
	0x00, 0xfc, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x06, 0x01, 0x80, 0x00, 
	0x0c, 0x00, 0xc0, 0x00, 0x1c, 0x00, 0x60, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x60, 0x00, 0x3e, 0x00, 
	0xc0, 0x00, 0x03, 0x00, 0xc0, 0x00, 0x01, 0x80, 0xe0, 0x00, 0x01, 0x80, 0x70, 0x00, 0x01, 0x80, 
	0x3f, 0xff, 0xff, 0x00, 0x1f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x06, 0x00, 
	0x03, 0x18, 0xc6, 0x00, 0x33, 0x18, 0xc6, 0x00, 0x33, 0x00, 0xc0, 0x00, 0x30, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x18, 0x00, 0x0c, 0x63, 0x18, 0x00, 0x0c, 0x63, 0x18, 0x00, 
	0x0c, 0x03, 0x00, 0x00
};

//TODO: Need to test

// 'freezingRain', 25x25px
const unsigned char freezingRain [] PROGMEM = {
	0x00, 0xfc, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x06, 0x01, 0x80, 0x00, 
	0x0c, 0x00, 0xc0, 0x00, 0x1c, 0x00, 0x60, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x60, 0x00, 0x3e, 0x00, 
	0xc0, 0x00, 0x03, 0x00, 0xc0, 0x00, 0x01, 0x80, 0xe0, 0x00, 0x01, 0x80, 0x70, 0x00, 0x01, 0x80, 
	0x3f, 0xff, 0xff, 0x00, 0x1f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x06, 0x00, 
	0x08, 0x18, 0x06, 0x00, 0x6b, 0x18, 0xc6, 0x00, 0x3e, 0x00, 0xc0, 0x00, 0x1c, 0x00, 0xc0, 0x00, 
	0x3e, 0x30, 0x0c, 0x00, 0x6b, 0x30, 0x0c, 0x00, 0x08, 0x31, 0x8c, 0x00, 0x00, 0x01, 0x80, 0x00, 
	0x00, 0x01, 0x80, 0x00
};

// 'snow', 25x25px
const unsigned char snow [] PROGMEM = {
	0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0xcc, 0xc0, 0x00, 0x00, 0xed, 0xc0, 0x00, 
	0x00, 0x7f, 0x88, 0x00, 0xc4, 0x3f, 0x19, 0x80, 0xe6, 0x1e, 0x1b, 0x80, 0x76, 0x0c, 0x1f, 0x00, 
	0x3e, 0x0c, 0x1e, 0x00, 0x1f, 0x0c, 0x1c, 0x00, 0x3f, 0xcc, 0xff, 0x00, 0x71, 0xff, 0xe7, 0x80, 
	0x60, 0x7f, 0x01, 0x80, 0x00, 0x1e, 0x00, 0x00, 0xc0, 0x7f, 0x01, 0x80, 0xf1, 0xff, 0xc7, 0x80, 
	0x3f, 0xcc, 0xff, 0x00, 0x0f, 0x0c, 0x3c, 0x00, 0x3e, 0x0c, 0x1e, 0x00, 0xf6, 0x0c, 0x1f, 0x80, 
	0xc6, 0x1e, 0x19, 0x80, 0x06, 0x3f, 0x18, 0x00, 0x04, 0x7f, 0x88, 0x00, 0x00, 0xed, 0xc0, 0x00, 
	0x00, 0xcc, 0xc0, 0x00
};

// 'thunderstorm', 25x25px
const unsigned char thunderstorm [] PROGMEM = {
	0x00, 0xfc, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x06, 0x01, 0x80, 0x00, 
	0x0c, 0x00, 0xc0, 0x00, 0x1c, 0x00, 0x60, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x60, 0x00, 0x3e, 0x00, 
	0xc0, 0x00, 0x03, 0x00, 0xc0, 0x00, 0x01, 0x80, 0xe0, 0x40, 0x01, 0x80, 0x70, 0xc0, 0x01, 0x80, 
	0x39, 0x9f, 0xff, 0x00, 0x13, 0xbf, 0xfe, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0f, 0x0c, 0x03, 0x00, 
	0x1e, 0x0c, 0x63, 0x00, 0x3f, 0xec, 0x63, 0x00, 0x03, 0xc0, 0x60, 0x00, 0x07, 0x80, 0x00, 0x00, 
	0x07, 0x00, 0x00, 0x00, 0x0e, 0x30, 0x0c, 0x00, 0x1c, 0x31, 0x8c, 0x00, 0x18, 0x31, 0x8c, 0x00, 
	0x30, 0x01, 0x80, 0x00
};

// 'blank', 25x25px
const unsigned char blank [] PROGMEM = { }; 

// 'city', 200x80px
const unsigned char city [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xf8, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfe, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 
	0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x1f, 0xe0, 0x0f, 0xfe, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x1f, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x1c, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xe0, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0xc1, 0x04, 0x01, 
	0xff, 0xf0, 0x00, 0x00, 0x1f, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x70, 0x00, 0x00, 0x01, 0x80, 0xf8, 0x01, 0xff, 0xf0, 0x00, 0x00, 0x1f, 0xb7, 0xe0, 
	0x07, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x01, 0x80, 
	0xf8, 0x01, 0xff, 0xf0, 0x00, 0x00, 0x1f, 0xb7, 0xe0, 0x07, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x01, 0xff, 0xf0, 0x00, 0x00, 0x1f, 
	0xff, 0xe0, 0x07, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 
	0x06, 0x00, 0x00, 0x01, 0xff, 0xf0, 0x00, 0x00, 0x1f, 0xff, 0xe0, 0x07, 0xff, 0xf0, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x01, 0xff, 0xf0, 0x00, 
	0x00, 0x1f, 0xb7, 0xe0, 0x07, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 
	0x00, 0x00, 0x0c, 0x00, 0x00, 0x01, 0xb4, 0xf0, 0x00, 0x00, 0x1f, 0xff, 0xe0, 0x07, 0xff, 0xf0, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x01, 0xff, 
	0xf0, 0x00, 0x00, 0x1f, 0xff, 0xe0, 0x07, 0xdb, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0x80, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x01, 0xff, 0xf0, 0x00, 0x00, 0x1f, 0xb7, 0xe0, 0x07, 
	0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x0c, 0x0f, 0xfe, 
	0x01, 0xb4, 0xf0, 0x03, 0xff, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xf0, 0x0f, 0xfe, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x0c, 0x0f, 0xfe, 0x01, 0xff, 0xf0, 0x03, 0xff, 0xff, 0xff, 
	0xe0, 0x07, 0xdb, 0x70, 0x0f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x0c, 
	0x0f, 0xfe, 0x01, 0xff, 0xf0, 0x03, 0xdb, 0xff, 0xb7, 0xe0, 0x07, 0xff, 0xf0, 0x0e, 0x6e, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x0c, 0x0f, 0xfe, 0x01, 0xb4, 0xf0, 0x03, 0xff, 
	0xff, 0xff, 0xe0, 0x07, 0xff, 0xf0, 0x0f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 
	0x00, 0x0c, 0x0f, 0xfe, 0x01, 0xff, 0xf0, 0x03, 0xdb, 0xff, 0xff, 0xff, 0x07, 0xdb, 0x70, 0x0e, 
	0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x0c, 0x0e, 0x5e, 0x01, 0xff, 0xf0, 
	0x03, 0xff, 0xff, 0xb7, 0xff, 0x07, 0xff, 0xf0, 0x0f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
	0x80, 0x00, 0x00, 0x0c, 0x0f, 0xfe, 0x01, 0xff, 0xf0, 0x03, 0xdb, 0xff, 0xff, 0xfb, 0x07, 0xff, 
	0xf0, 0x0e, 0x6e, 0x00, 0xff, 0xc0, 0x00, 0x00, 0x01, 0x80, 0x1f, 0xff, 0x0c, 0x0f, 0xfe, 0x01, 
	0xf7, 0xff, 0x03, 0xff, 0xff, 0xff, 0xff, 0x07, 0xdb, 0xf0, 0x0f, 0xfe, 0x00, 0xff, 0xc0, 0xff, 
	0xe0, 0x01, 0x80, 0x1f, 0xff, 0x0c, 0x0f, 0xfe, 0x01, 0xff, 0xff, 0xff, 0xdb, 0xff, 0xb7, 0xfb, 
	0x07, 0xff, 0xf0, 0x0e, 0x6e, 0x00, 0xec, 0xc0, 0xff, 0xe0, 0x01, 0x80, 0x1f, 0xff, 0x0c, 0x0f, 
	0xfe, 0x01, 0xff, 0xff, 0xff, 0xdb, 0xff, 0xb7, 0xfb, 0x07, 0xff, 0xf0, 0x0e, 0x6e, 0x00, 0xec, 
	0xc0, 0xff, 0xe0, 0x01, 0x80, 0x1f, 0xff, 0x0c, 0x0e, 0x5e, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x07, 0xff, 0xf0, 0x0f, 0xfe, 0x00, 0xff, 0xc0, 0xff, 0xe0, 0x01, 0x80, 0x1f, 0xff, 
	0x0c, 0x0f, 0xfe, 0x01, 0xff, 0xff, 0xff, 0xdb, 0xff, 0xff, 0xff, 0x07, 0xff, 0xff, 0xff, 0xfe, 
	0x00, 0xff, 0xc0, 0xed, 0xe0, 0x01, 0x80, 0x1f, 0xff, 0x0c, 0x0f, 0xff, 0xff, 0xf7, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0xff, 0xc0, 0xed, 0xe0, 0x01, 0x9f, 
	0xff, 0xff, 0x0c, 0x0f, 0xff, 0xff, 0xff, 0xfb, 0x7f, 0xdb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xe0, 0x01, 0x9f, 0xff, 0xff, 0x0c, 0x0e, 0x5f, 0xb7, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xe0, 
	0x01, 0x9f, 0xff, 0xff, 0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xed, 0xff, 0xfd, 0xbe, 0xfb, 0x6f, 0xed, 0xff, 0xff, 
	0xff, 0xf7, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
	0xed, 0xff, 0xfd, 0xbe, 0xfb, 0x6f, 0xed, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x7f, 0xfd, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xfd, 0xbf, 0xff, 0xff, 0xed, 
	0xfe, 0x5f, 0xb7, 0xff, 0xff, 0xff, 0xfd, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x0e, 0xff, 0xff, 0xfd, 0xbf, 0xff, 0xff, 0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xfd, 0xff, 0xfd, 0xb6, 0xdb, 
	0x6d, 0xed, 0xff, 0xff, 0xff, 0xf7, 0xfb, 0x7f, 0xdb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x0e, 0xfd, 0xff, 0xfd, 0xb6, 0xdb, 0x6d, 0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xff, 0xff, 0xfd, 
	0xbf, 0xff, 0xff, 0xed, 0xfe, 0x5f, 0xbf, 0xff, 0xff, 0xff, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xff, 0xff, 0xfd, 0xbf, 0xff, 0xff, 0xed, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xff, 
	0xf9, 0xfd, 0xbf, 0xff, 0xff, 0xed, 0xff, 0xff, 0xff, 0xf7, 0xfb, 0x7f, 0xfb, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xff, 0xf9, 0xfd, 0xbf, 0xff, 0xff, 0xed, 0xff, 
	0xff, 0xff, 0xf7, 0xfb, 0x7f, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x0e, 0xff, 0xff, 0xfd, 0xbf, 0xff, 0xff, 0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xf3, 0xff, 0xfd, 0xbf, 0xff, 0xff, 
	0xcc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x0e, 0x0c, 0xff, 0xfd, 0xbf, 0xff, 0xff, 0xcc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xfe, 0x7f, 0x9d, 0xbf, 
	0xff, 0xff, 0xde, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x7f, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0e, 0xff, 0x9e, 0x6d, 0xbe, 0x1f, 0x01, 0xde, 0xf0, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xff, 0xef, 
	0xfd, 0x9c, 0xee, 0x7e, 0xde, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xff, 0xef, 0xfd, 0x9d, 0xee, 0xff, 0xde, 0xff, 0x7f, 
	0xff, 0xff, 0xff, 0xff, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 
	0xdf, 0xf3, 0xfd, 0x9f, 0xf7, 0xff, 0xde, 0xff, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xff, 0xf9, 0xfd, 0x9f, 0xff, 0xff, 0xde, 
	0xff, 0xff, 0xfb, 0xff, 0xff, 0x0f, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x0e, 0xff, 0xdd, 0xfd, 0xbf, 0xff, 0xc7, 0x9e, 0x00, 0x0f, 0xe3, 0xff, 0x7f, 0x0f, 0xfb, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xff, 0xff, 0xf9, 0x9f, 0xfc, 
	0x00, 0x1e, 0x00, 0x3f, 0x83, 0xfe, 0x7f, 0x83, 0xfb, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x0e, 0xff, 0xff, 0xc1, 0x9c, 0x00, 0x00, 0x1e, 0x01, 0xfe, 0x0f, 0xff, 0xff, 
	0xe1, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xc0, 0x03, 
	0xcc, 0x00, 0x08, 0x3f, 0xc7, 0xf0, 0x3f, 0xff, 0xff, 0xf0, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xc0, 0x23, 0xcc, 0x04, 0x00, 0x9f, 0xff, 0xc0, 0xff, 
	0xfc, 0xff, 0xf8, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 
	0x04, 0x03, 0xcc, 0x00, 0x80, 0x9f, 0xff, 0x03, 0xff, 0xf8, 0xff, 0xfe, 0x1f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x80, 0x00, 0x03, 0xcc, 0x00, 0x00, 0x9f, 0xfc, 
	0x0f, 0xff, 0xf9, 0xff, 0xff, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x03, 0xf8, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x83, 0xc0, 0x01, 0xff, 0xcf, 0xf0, 0x3f, 0xff, 0xf1, 0xff, 0xff, 0x83, 0xe2, 
	0x00, 0x10, 0x00, 0x23, 0x38, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x83, 0xc0, 0x83, 0xfe, 
	0x3f, 0xc0, 0xff, 0xff, 0xf1, 0xff, 0xff, 0xe1, 0xf0, 0x40, 0x00, 0x40, 0x03, 0x38, 0x00, 0x40, 
	0x00, 0x04, 0x04, 0x08, 0x00, 0x03, 0xc0, 0x06, 0xbe, 0xfe, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xf0, 0x7e, 0x50, 0x00, 0x00, 0x03, 0x38, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x20, 0x43, 0xc9, 
	0x06, 0xbf, 0xf8, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x3e, 0x40, 0x00, 0x06, 0x03, 0x38, 
	0x04, 0x00, 0x21, 0x00, 0x08, 0x00, 0x00, 0x03, 0xc0, 0x0c, 0x7f, 0xe0, 0x7f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xfe, 0x1f, 0xe1, 0x0a, 0x00, 0x03, 0x38, 0x80, 0x04, 0x04, 0x00, 0x20, 0x2c, 0x42, 
	0x1b, 0xc0, 0x0d, 0x7f, 0x81, 0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0x07, 0xe8, 0x60, 0x00, 
	0x0b, 0x38, 0xb0, 0x25, 0x80, 0x04, 0x00, 0xb8, 0x02, 0xdb, 0xcb, 0x1d, 0xfe, 0x07, 0xff, 0xff, 
	0xff, 0x87, 0xff, 0xff, 0xff, 0x83, 0xfa, 0x60, 0xc0, 0x03, 0x38, 0x30, 0x21, 0x80, 0x01, 0x00, 
	0x80, 0x18, 0xdb, 0xcf, 0x7f, 0xf8, 0x1f, 0xff, 0xff, 0xff, 0x87, 0xff, 0xff, 0xff, 0xe1, 0xfb, 
	0xd9, 0xde, 0x03, 0x38, 0x01, 0xa0, 0x01, 0xb1, 0x67, 0x06, 0x1d, 0xc3, 0xcf, 0x7f, 0xe0, 0x7f, 
	0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xf0, 0x7f, 0x58, 0x1e, 0x0b, 0x38, 0x01, 0xa0, 0x05, 
	0xb3, 0x61, 0x1c, 0xdf, 0xe3, 0xc7, 0x7f, 0x01, 0xff, 0xff, 0xff, 0xfe, 0x0f, 0xff, 0xff, 0xff, 
	0xf8, 0x3f, 0x58, 0x1a, 0xcb, 0x38, 0x31, 0x81, 0x84, 0x06, 0x00, 0xd8, 0xc7, 0xeb, 0xc7, 0xfc, 
	0x07, 0xff, 0xff, 0xff, 0xfe, 0x1f, 0xff, 0xff, 0xff, 0xfe, 0x0f, 0xda, 0xd0, 0x0b, 0x38, 0x36, 
	0x01, 0xb0, 0x02, 0x04, 0x0b, 0xe7, 0xfb, 0xcf, 0xf0, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x07, 0xfa, 0x80, 0x03, 0x38, 0x66, 0x13, 0x31, 0x13, 0xb4, 0x0f, 0xef, 0xff, 
	0xff, 0xc0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83, 0xfe, 0x8b, 0x63, 
	0x39, 0x42, 0xda, 0x13, 0x33, 0x94, 0xcf, 0xff, 0xff, 0xff, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0xfe, 0xdb, 0x63, 0x3b, 0x42, 0x7a, 0x1b, 0x77, 0x9e, 0xdf, 
	0xfe, 0xff, 0xfc, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x7f, 
	0xdb, 0x6f, 0x3b, 0x7b, 0x1b, 0xdb, 0x77, 0x77, 0xdf, 0xfe, 0xff, 0xf0, 0x1f, 0xff, 0xff, 0xff, 
	0xff, 0xf8, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x3f, 0xdb, 0x6f, 0x7b, 0x7b, 0x1b, 0xdb, 0x77, 
	0x77, 0xf6, 0xbf, 0xff, 0xc0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x1f, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x0f, 0xfc, 0x2f, 0x7b, 0xfe, 0x7f, 0xff, 0x61, 0x6f, 0xf6, 0x9f, 0xfe, 0x03, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xe0, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0xfc, 0x27, 0x7b, 0x0e, 0xf8, 
	0x7a, 0x09, 0x6f, 0x3f, 0xdf, 0xf8, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x3f, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x83, 0xfe, 0xb7, 0x79, 0x0f, 0x68, 0x72, 0x9b, 0x7f, 0x1f, 0xff, 0xe0, 0x3f, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0xfe, 0xff, 0x79, 
	0xff, 0x6f, 0xf7, 0xfb, 0x5f, 0xdf, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x7f, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x7f, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xdf, 0x5b, 0xff, 0xfe, 
	0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x1f, 
	0xff, 0x7f, 0xff, 0xff, 0xff, 0xdf, 0xfb, 0xff, 0xf0, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff
};
