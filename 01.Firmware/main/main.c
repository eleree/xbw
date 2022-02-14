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

#if 0
char *osd_getromdata() {
	char* romdata;
	const esp_partition_t* part;
	spi_flash_mmap_handle_t hrom;
	esp_err_t err;
	nvs_flash_init();
	part=esp_partition_find_first(0x40, 1, NULL);
	if (part==0) printf("Couldn't find rom part!\n");
	err=esp_partition_mmap(part, 0, 3*1024*1024, SPI_FLASH_MMAP_DATA, (const void**)&romdata, &hrom);
	if (err!=ESP_OK) printf("Couldn't map rom part!\n");
	printf("Initialized. ROM@%p\n", romdata);
    return (char*)romdata;
}
#else
const char *SD_BASE_PATH = "/sd";
static char *ROM_DATA = (char *)0x3f800000;

char *osd_getromdata(void)
{
	printf("Initialized. ROM@%p\n", ROM_DATA);
	return (char *)ROM_DATA;
}

#endif

esp_err_t event_handler(void *ctx, system_event_t *event)
{
	return ESP_OK;
}

void sdspi_test(void)
{
	vTaskDelay(3000);
	sdcard_open("/sd");
	int count = sdcard_get_files_count("/sd/sd");
	printf("sd card files :%d\r\n", count);
	size_t file_size = sdcard_get_filesize("/sd/sd/NOTICE.txt");
	printf("sd card file size :%d\r\n", file_size);
}

void ui_init(void);
char *ui_file_chooser(const char *path, const char *filter, int currentItem, char *title);
void ui_clear_screen(void);

char *romPath = NULL;
void vfs_test(void)
{
	FILE *f = fopen("/sd/hello.txt", "w");
	if (f == NULL)
	{
		return;
	}
	fprintf(f, "Hello %s!\n", "world");
	fclose(f);
}
void load_rom(void)
{
	esp_err_t ret = sdcard_open(SD_BASE_PATH);
	if (ret != ESP_OK)
	{
		printf("failed to open sd card\n");
		return NULL;
	}
	romPath = ui_file_chooser("/sd/roms", ".nes", 0, "Select ROM");
	if (romPath == NULL)
	{
		printf("No ROM selected\n");
		return;
	}
	printf("ROM: %s\n", romPath);
	sdcard_close();
	ROM_DATA = malloc(3*1024*1024);
	if (!romPath)
	{
		printf("osd_getromdata: Reading from flash.\n");

		// copy from flash
		// spi_flash_mmap_handle_t hrom;
		const esp_partition_t *part = esp_partition_find_first(0x40, 1, NULL);
		if (part == 0)
		{
			printf("esp_partition_find_first failed.\n");
			abort();
		}

		esp_err_t err = esp_partition_read(part, 0, (void *)ROM_DATA, 0x100000);
		if (err != ESP_OK)
		{
			printf("esp_partition_read failed. size = %x (%d)\n", part->size, err);
			abort();
		}
	}
	else
	{
		printf("osd_getromdata: Reading from sdcard.\n");

		// copy from SD card
		esp_err_t r = sdcard_open(SD_BASE_PATH);
		if (r != ESP_OK)
		{
			abort();
		}

		// display_clear(0);
		// display_show_hourglass();
		size_t file_size = sdcard_get_filesize(romPath);
		printf("sd card file size :%d\r\n", file_size);
		vfs_test();
		size_t fileSize = sdcard_copy_file_to_memory(romPath, ROM_DATA);
		printf("app_main: fileSize=%d\n", fileSize);
		if (fileSize == 0)
		{
			abort();
		}

		r = sdcard_close();
		if (r != ESP_OK)
		{
			abort();
		}

		free(romPath);
	}
}

int app_main(void)
{
	vga_init();
	vga_write_frame(0, 0, 320, 240, NULL);
	ui_init();
	load_rom();
	ui_clear_screen();
	printf("starting esp32 nofrendo\r\n");
	printf("NoFrendo start!\n");
	nofrendo_main(0, NULL);
	printf("NoFrendo died? WtF?\n");
	asm("break.n 1");
	return 0;
}
