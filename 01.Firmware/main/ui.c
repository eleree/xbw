#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "nofrendo.h"
#include "esp_partition.h"
#include "sdcard.h"
#include <vga16.h>
#include "joystck.h"
#include "ugui.h"
#include <string.h>

#define SD_CARD "/sd"
static UG_GUI *ugui;

static void pset(UG_S16 x, UG_S16 y, UG_COLOR color)
{
  vga_write_pixel(x, y, color);
}

void ui_clear_screen(void)
{
  for (int y = 0; y < 240; y++)
    for (int x = 0; x < 320; x++)
      vga_write_pixel(x, y, C_BLACK);
}

void ui_flush(void)
{
}

#define MAX_CHR (320 / 9)
#define MAX_ITEM (193 / 15) // 193 = LCD Height (240) - header(16) - footer(16) - char height (15)

/// Make the file name nicer by cutting at brackets or (last) dot.
static int cut_file_name(char *filename)
{

  char *dot = strrchr(filename, '.');
  char *brack1 = strchr(filename, '[');
  char *brack2 = strchr(filename, '(');

  int len = strlen(filename);
  if (dot != NULL && dot - filename < len)
  {
    len = dot - filename;
  }
  if (brack1 != NULL && brack1 - filename < len)
  {
    len = brack1 - filename;
    if (filename[len - 1] == ' ')
    {
      len--;
    }
  }
  if (brack2 != NULL && brack2 - filename < len)
  {
    len = brack2 - filename;
    if (filename[len - 1] == ' ')
    {
      len--;
    }
  }
  return len;
}

static void ui_draw_page_list(char **files, int fileCount, int currentItem, int extLen, char *title)
{
  /* Header */
  UG_FillFrame(0, 0, 320 - 1, 16 - 1, C_BLUE);
  UG_SetForecolor(C_WHITE);
  UG_SetBackcolor(C_BLUE);
  char *msg = title;
  UG_PutString((320 / 2) - (strlen(msg) * 9 / 2), 2, msg);
  /* End Header */

  /* Footer */
  UG_FillFrame(0, 240 - 16 - 1, 320 - 1, 240 - 1, C_BLUE);
  UG_SetForecolor(C_WHITE);
  UG_SetBackcolor(C_BLUE);
  msg = "  Load    Back    PgUp    PgDown";
  UG_PutString((320 / 2) - (strlen(msg) * 9 / 2), 240 - 15, msg);

  UG_FillCircle(22, 240 - 10, 7, C_WHITE);
  UG_SetForecolor(C_BLACK);
  UG_SetBackcolor(C_WHITE);
  UG_PutString(20, 240 - 15, "A");

  UG_FillCircle(95, 240 - 10, 7, C_WHITE);
  UG_PutString(92, 240 - 15, "B");

  UG_FillCircle(168, 240 - 10, 7, C_WHITE);
  UG_PutString(165, 240 - 15, "<");

  UG_FillCircle(240, 240 - 10, 7, C_WHITE);
  UG_PutString(237, 240 - 15, ">");
  /* End Footer */

  const int innerHeight = 193;
  int page = currentItem / MAX_ITEM;
  page *= MAX_ITEM;
  const int itemHeight = innerHeight / MAX_ITEM;

  UG_FillFrame(0, 15, 320 - 1, 193 + 15, C_BLACK);

  if (fileCount < 1)
  {
    UG_SetForecolor(C_RED);
    UG_SetBackcolor(C_BLACK);
    msg = "No Files Found";
    UG_PutString((320 / 2) - (strlen(msg) * 9 / 2), (240 - 16 - 13) / 2, msg);
    ui_flush();
  }
  else
  {
    char *displayStr[MAX_ITEM];
    for (int i = 0; i < MAX_ITEM; ++i)
    {
      displayStr[i] = NULL;
    }

    for (int line = 0; line < MAX_ITEM; ++line)
    {
      if (page + line >= fileCount)
        break;
      short top = 19 + (line * itemHeight) - 1;
      if ((page) + line == currentItem)
      {
        UG_FillFrame(0, top - 1, 320 - 1, top + 12 + 1, C_YELLOW);
        UG_SetForecolor(C_BLACK);
        UG_SetBackcolor(C_YELLOW);
      }
      else
      {
        UG_SetForecolor(C_WHITE);
        UG_SetBackcolor(C_BLACK);
      }

      char *filename = files[page + line];
      if (!filename)
        abort();
      int length = cut_file_name(filename);
      displayStr[line] = (char *)malloc(length + 1);
      strncpy(displayStr[line], filename, length);
      displayStr[line][length] = 0;
      char truncnm[MAX_CHR];
      strncpy(truncnm, displayStr[line], MAX_CHR);
      truncnm[MAX_CHR - 1] = 0;
      UG_PutString((320 / 2) - (strlen(truncnm) * 9 / 2), top, truncnm);
    }
    ui_flush();
    for (int i = 0; i < MAX_ITEM; ++i)
    {
      free(displayStr[i]);
    }
  }
}

enum
{
  GAMEPAD_INPUT_START = 0,
  GAMEPAD_INPUT_SELECT,
  GAMEPAD_INPUT_UP,
  GAMEPAD_INPUT_DOWN,
  GAMEPAD_INPUT_LEFT,
  GAMEPAD_INPUT_RIGHT,
  GAMEPAD_INPUT_A,
  GAMEPAD_INPUT_B,
  GAMEPAD_INPUT_MENU,
  GAMEPAD_INPUT_L,
  GAMEPAD_INPUT_R,
  GAMEPAD_INPUT_MAX
};

typedef struct
{
  uint8_t values[GAMEPAD_INPUT_MAX];
} input_gamepad_state;

void gamepad_read(input_gamepad_state *out_state)
{
  int key = joystickReadInput();
  //printf("key: %d\n", key);
  if ((key & 0X01) == 0x00)
    out_state->values[GAMEPAD_INPUT_RIGHT] = 1;
  else
    out_state->values[GAMEPAD_INPUT_RIGHT] = 0;

  if ((key & 0X02) == 0x00)
    out_state->values[GAMEPAD_INPUT_LEFT] = 1;
  else
    out_state->values[GAMEPAD_INPUT_LEFT] = 0;

  if ((key & 0X04) == 0x00)
    out_state->values[GAMEPAD_INPUT_DOWN] = 1;
  else
    out_state->values[GAMEPAD_INPUT_DOWN] = 0;

  if ((key & 0X08) == 0x00)
    out_state->values[GAMEPAD_INPUT_UP] = 1;
  else
    out_state->values[GAMEPAD_INPUT_UP] = 0;

  if ((key & 0X10) == 0x00)
    out_state->values[GAMEPAD_INPUT_START] = 1;
  else
    out_state->values[GAMEPAD_INPUT_START] = 0;

  if ((key & 0X20) == 0x00)
    out_state->values[GAMEPAD_INPUT_SELECT] = 1;
  else
    out_state->values[GAMEPAD_INPUT_SELECT] = 0;

  if ((key & 0X40) == 0x00)
    out_state->values[GAMEPAD_INPUT_B] = 1;
  else
    out_state->values[GAMEPAD_INPUT_B] = 0;

  if ((key & 0X80) == 0x00)
    out_state->values[GAMEPAD_INPUT_A] = 1;
  else
    out_state->values[GAMEPAD_INPUT_A] = 0;
}

char *ui_select_file(void)
{
  char *title = "title";
  char *path = "/sd/sd";
  char *filter = ".txt";
  char **files = 0;
  int extLen = strlen(filter);
  int selected = 0;
  esp_err_t ret = sdcard_open(SD_CARD);
  if (ret != ESP_OK)
  {
    printf("failed to open sd card\n");
    return NULL;
  }

  int fileCount = sdcard_files_get(path, filter, &files);
  printf("%s: fileCount = %d\n", __func__, fileCount);
  char *msg = "XBW NES ROMs";
  UG_PutString((320 / 2) - (strlen(msg) * 9 / 2), 2, msg);

  ui_draw_page_list(files, fileCount, selected, extLen, title);
  while (1)
  {
    input_gamepad_state key;
    gamepad_read(&key);

    vTaskDelay(1000);
  }
  sdcard_files_free(files, fileCount);

  return NULL;
}

char *ui_file_chooser(const char *path, const char *filter, int currentItem, char *title)
{
  char *result = NULL;
  int extLen = strlen(filter);
  char **files = 0;
  int fileCount = sdcard_files_get(path, filter, &files);
  printf("%s: fileCount = %d\n", __func__, fileCount);

  int selected = currentItem;
  ui_draw_page_list(files, fileCount, selected, extLen, title);

  input_gamepad_state prevKey;
  gamepad_read(&prevKey);

  while (true)
  {
    input_gamepad_state key;
    gamepad_read(&key);

    int page = selected / MAX_ITEM;
    page *= MAX_ITEM;

    if (fileCount > 0)
    {
      if (!prevKey.values[GAMEPAD_INPUT_DOWN] && key.values[GAMEPAD_INPUT_DOWN])
      {
        if (selected + 1 < fileCount)
        {
          ++selected;
          ui_draw_page_list(files, fileCount, selected, extLen, title);
        }
        else
        {
          currentItem = 0;
          ui_draw_page_list(files, fileCount, selected, extLen, title);
        }
      }
      else if (!prevKey.values[GAMEPAD_INPUT_UP] && key.values[GAMEPAD_INPUT_UP])
      {
        if (selected > 0)
        {
          --selected;
          ui_draw_page_list(files, fileCount, selected, extLen, title);
        }
        else
        {
          selected = fileCount - 1;
          ui_draw_page_list(files, fileCount, selected, extLen, title);
        }
      }
      else if (!prevKey.values[GAMEPAD_INPUT_RIGHT] && key.values[GAMEPAD_INPUT_RIGHT])
      {
        if (page + MAX_ITEM < fileCount)
        {
          selected = page + MAX_ITEM;
          ui_draw_page_list(files, fileCount, selected, extLen, title);
        }
        else
        {
          selected = 0;
          ui_draw_page_list(files, fileCount, selected, extLen, title);
        }
      }
      else if (!prevKey.values[GAMEPAD_INPUT_LEFT] && key.values[GAMEPAD_INPUT_LEFT])
      {
        if (page - MAX_ITEM > 0)
        {
          selected = page - MAX_ITEM;
          ui_draw_page_list(files, fileCount, selected, extLen, title);
        }
        else
        {
          selected = page;
          while (selected + MAX_ITEM < fileCount)
          {
            selected += MAX_ITEM;
          }
          ui_draw_page_list(files, fileCount, selected, extLen, title);
        }
      }
      else if (!prevKey.values[GAMEPAD_INPUT_A] && key.values[GAMEPAD_INPUT_A])
      {
        if (fileCount < 1)
        {
          vTaskDelay(10);
          break;
        }

        size_t fullPathLength = strlen(path) + 1 + strlen(files[selected]) + 1;

        char *fullPath = (char *)malloc(fullPathLength);
        if (!fullPath)
          abort();

        strcpy(fullPath, path);
        strcat(fullPath, "/");
        strcat(fullPath, files[selected]);

        ui_clear_screen();
        ui_flush();

        result = fullPath;
        break;
      }
      else if (!prevKey.values[GAMEPAD_INPUT_B] && key.values[GAMEPAD_INPUT_B])
      {
        vTaskDelay(10);
        break;
      }
    }
    else
    {
      if (!prevKey.values[GAMEPAD_INPUT_B] && key.values[GAMEPAD_INPUT_B])
      {
        vTaskDelay(10);
        break;
      }
    }

    prevKey = key;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  sdcard_files_free(files, fileCount);

  return result;
}

void ui_init(void)
{
  joystickInit();
  ugui = malloc(sizeof(UG_GUI));
  UG_Init(ugui, pset, 320, 240);
  UG_FontSelect(&FONT_8X12);
  ui_clear_screen();

  //ui_select_file();
}

void ui_deinit(void)
{
  free(ugui);
  ugui = NULL;
}
