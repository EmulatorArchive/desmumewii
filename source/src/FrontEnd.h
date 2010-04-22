// Placeholder file for later

//--DCN: Copied from earlier version of desmumeWii
#ifndef FRONTEND
#define FRONTEND

  u16 arm9_gdb_port;
  u16 arm7_gdb_port;

  int enable_sound;
  int disable_limiter;
  int cpu_ratio;
  int lang;
  int showfps = 0;
  int vertical = 1;
  int frameskip;
  const char *nds_file;
  const char *cflash_disk_image_file;


// Screen layout/scale etc...
enum {
	SCREEN_VERT_NORMAL = 0,
	SCREEN_VERT_SEPARATED,
	SCREEN_HORI_NORMAL,
	SCREEN_VERT_STRETCH,
	SCREEN_HORI_STRETCH,
	SCREEN_MAIN_NORMAL,
	SCREEN_SUB_NORMAL,
	SCREEN_MAIN_STRETCH,
	SCREEN_SUB_STRETCH,
	SCREEN_MAX
};

u32 screen_layout = SCREEN_MAX;

// positioning screen vars.
int bottomX, bottomY, topX, topY;
float scalex, scaley;
float width = 256;
float height = 192;

void DoConfig();

void DSEmuGui(char *path,char *out);

#endif
//--DCN: End copy

