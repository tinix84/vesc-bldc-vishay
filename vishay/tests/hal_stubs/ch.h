#ifndef VISHAY_TEST_CH_H
#define VISHAY_TEST_CH_H
#include <stddef.h>
#include <stdint.h>
typedef uint32_t systime_t;
typedef struct { int unused; } thread_t;
#define THD_WORKING_AREA(name, size) uint8_t name[size]
#define THD_FUNCTION(name, arg) void name(void *arg)
#define NORMALPRIO 10
#define US2ST(us) (((uint32_t)(us) + 99U) / 100U)
void chRegSetThreadName(const char *name);
void chThdSleepMilliseconds(uint32_t ms);
void chThdSleepMicroseconds(uint32_t us);
thread_t *chThdCreateStatic(void *area, size_t size, int priority, void (*entry)(void *), void *arg);
systime_t chVTGetSystemTimeX(void);
systime_t chVTTimeElapsedSinceX(systime_t previous);
#endif
