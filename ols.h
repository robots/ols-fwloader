/*
 * Michal Demin - 2010
 */
#ifndef OLS_H_
#define OLS_H_

#include <stdint.h>

struct ols_flash_t {
	const char *jedec_id;
	uint16_t page_size;
	uint16_t pages;
	const char *name;
};

struct ols_t {
	int fd;
	struct ols_flash_t *flash;
	int verbose;
};


struct ols_t *OLS_Init(char *, unsigned long); 
int OLS_Deinit(struct ols_t *);
int OLS_RunSelftest(struct ols_t *);
int OLS_GetStatus(struct ols_t *);
int OLS_GetID(struct ols_t *);
int OLS_EnterBootloader(struct ols_t *);
int OLS_EnterRunMode(struct ols_t *);
int OLS_GetFlashID(struct ols_t *);
struct ols_flash_t *OLS_GetFlash(struct ols_t *);
int OLS_FlashErase(struct ols_t *);
int OLS_FlashRead(struct ols_t *, uint16_t page, uint8_t *buf);
int OLS_FlashWrite(struct ols_t *, uint16_t page, uint8_t *buf);

#endif

