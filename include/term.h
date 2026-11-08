#ifndef SP_TERM_H
#define SP_TERM_H

#include <stdbool.h>
#include <stdint.h>

void term_init(void);
void term_shutdown(void);
void term_enter_raw(void);
void term_leave_raw(void);
int term_read_key(void);
uint64_t term_now_ms(void);
void term_sleep_ms(int ms);
void term_clear(void);
void term_hide_cursor(void);
void term_show_cursor(void);

#endif
