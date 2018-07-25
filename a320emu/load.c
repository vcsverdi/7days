#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "load.h"
#include "disasm.h"

static uint8_t *body=NULL;

uint32_t print_table(uint8_t *src, uint32_t off, uint32_t size)
{
  // find string table
  uint8_t *p = src + off;
  uint32_t str_off = off;
  uint32_t cnt, total = size/sizeof(_app_impt_entry);
	for(cnt=0; cnt<total; cnt++){
    if(((_app_impt_entry*)p)->unknown[1] > 0x20000){
      printf("  string: count(0x%x) offset(0x%x)\n", cnt, str_off);
      break;
    }
		p+= sizeof(_app_impt_entry);
    str_off+= sizeof(_app_impt_entry);
	}

  // print each import item
	p = src + off;
	for(cnt=0; cnt<total; cnt++, p+= sizeof(_app_impt_entry)){
    if(((_app_impt_entry*)p)->unknown[1] == 0x00000){
      continue;
    }
    if(((_app_impt_entry*)p)->unknown[1] > 0x20000){
      break;
    }
		printf("  0x%x(%s)\n", ((_app_impt_entry*)p)->offset, &src[((_app_impt_entry*)p)->str_offset + str_off]);
	}
  return str_off;
}

void free_app(void)
{
  if(body){
    free(body);
  }
}

uint8_t* load_app(const char* app, int dis)
{
	FILE* file = fopen(app, "rb");
  if(file == NULL) {
    printf("failed to open app: %s", app);
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  uint32_t len = ftell(file);
  fseek(file, 0, SEEK_SET);
  body = (uint8_t*)malloc(len);
  if(body == NULL) {
    printf("failed to allocate buffer for reading app\n");
    fclose(file);
    return NULL;
  }
  fread(body, 1, len, file);
  fclose(file); 
  printf("=== app info ===\n");

  // ccdl table
	uint8_t *ptr = body;
	if(memcmp(ptr, "CCDL", 4) != 0){
		printf("invalid app file format (miss CCDL section)\n");
		goto err;
	}
	ptr+= sizeof(_app_ccdl);

  // import table
	uint32_t off = ((_app_impt*)ptr)->offset;
	uint32_t size = ((_app_impt*)ptr)->size;
	if(memcmp(ptr, "IMPT", 4) != 0){
		printf("invalid app file format (miss IMPT section)\n");
		goto err;
	}
	printf("import offset: 0x%x\n", off);
	printf("import size: 0x%x\n", size);
  print_table(body, off, size);
	ptr+= sizeof(_app_impt);

  // export table
	if(memcmp(ptr, "EXPT", 4) != 0){
		printf("invalid app file format (miss EXPT section)\n");
		goto err;
	}
	off = ((_app_rawd*)ptr)->offset;
	size = ((_app_rawd*)ptr)->size;
	printf("export offset: %d\n", off);
	printf("export size: %d\n", size);
  print_table(body, off, size);
	ptr+= sizeof(_app_expt);
	
	if(memcmp(ptr, "RAWD", 4) != 0){
		printf("invalid app file format (miss RAWD section)\n");
		goto err;
	}

  // raw table
	off = ((_app_rawd*)ptr)->offset;
	size = ((_app_rawd*)ptr)->size;
	printf("raw offset: %d\n", off);
	printf("raw size: %d\n", size);
	printf("raw entry: 0x%x\n", ((_app_rawd*)ptr)->entry);
	printf("raw origin: 0x%x\n", ((_app_rawd*)ptr)->origin);
	printf("raw prog_size: %d\n", ((_app_rawd*)ptr)->prog_size);
	ptr+= sizeof(_app_rawd);
  printf("\n");

#if 0 
  const char* out="raw.bin";
	int fd = open(out, O_CREAT | O_WRONLY, S_IRUSR);
	if(fd < 0){
		printf("failed to create binary file: %s\n", out);
		goto err;
	}
	write(fd, body+off, size);
	close(fd);
#endif

  if(dis){
    uint8_t *p=body+off;

    printf("=== disasm ===\n");
    for(int i=0; i<size; i+=4, p+=4){
      printf("%08x(0x%08x): ", i, *(inst*)p);
      disasm(*(inst*)p);
    }
  }
  return body + off;

err:
	free(body);
  exit(-1);
  return NULL;
}

