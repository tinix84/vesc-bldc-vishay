/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "raik060_vesc.h"
#include "ch.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

static float (*read_cb)(void);
static bool (*fault_cb)(void);
static char *(*info_cb)(void);
static void (*thread_entry)(void *);
static systime_t now;
static bool create_ok;
static unsigned bit_index;
static uint64_t frame;
static jmp_buf sampled;
static thread_t thread;

void encoder_set_custom_callbacks(float (*read)(void), bool (*fault)(void), char *(*info)(void)) {
    read_cb=read; fault_cb=fault; info_cb=info;
}
void chRegSetThreadName(const char *name) { (void)name; }
void chThdSleepMilliseconds(uint32_t ms) { (void)ms; }
void chThdSleepMicroseconds(uint32_t us) { (void)us; longjmp(sampled,1); }
thread_t *chThdCreateStatic(void *area,size_t size,int priority,void (*entry)(void *),void *arg) {
    (void)area;(void)size;(void)priority;(void)arg; thread_entry=entry;
    return create_ok ? &thread : NULL;
}
systime_t chVTGetSystemTimeX(void) { return now; }
systime_t chVTTimeElapsedSinceX(systime_t previous) { return now-previous; }
static void cs(void *arg,bool high) { (void)arg;if(!high)bit_index=0; }
static void clock_pin(void *arg,bool high) { (void)arg;(void)high; }
static bool read_pin(void *arg) { (void)arg;return (frame>>(27U-bit_index++))&1U; }
static void delay(void *arg,uint32_t us) { (void)arg;(void)us; }
static void sample(uint64_t raw) {
    frame=raw;
    if(setjmp(sampled)==0)thread_entry(NULL);
}
int main(void) {
    raik060_vesc_config_t conf={0};
    conf.bus=(raik060_bus_t){NULL,cs,clock_pin,read_pin,delay,1};
    conf.frame_kind=RAIK060_FRAME_SINGLE_TURN;
    conf.frame_kind=(raik060_frame_kind_t)99;
    assert(!raik060_vesc_start(&conf));
    conf.frame_kind=RAIK060_FRAME_SINGLE_TURN;
    assert(!raik060_vesc_start(&conf)); /* Thread creation failure must remain faulty. */
    assert(fault_cb());
    create_ok=true;
    assert(raik060_vesc_start(&conf));
    assert(fault_cb()); /* No first sample yet. */
    const uint64_t payload=((uint64_t)65536<<2)|3U;
    const uint64_t good=(payload<<8)|raik060_crc8(payload,20);
    now=100;sample(good);
    assert(!fault_cb()); assert(read_cb()==90.0f); assert(info_cb()!=NULL);
    now+=US2ST(1000);
    assert(fault_cb()); /* Last valid sample became stale. */
    sample(good);
    assert(!fault_cb());
    sample(good^1U);
    assert(fault_cb()); /* A CRC failure inhibits immediately by default. */
    sample(good);
    assert(!fault_cb());
    const uint64_t error_payload=((uint64_t)65536<<2)|1U;
    sample((error_payload<<8)|raik060_crc8(error_payload,20));
    assert(fault_cb());
    now=UINT32_MAX-5U;sample(good);
    assert(!fault_cb());
    now=5U;
    assert(fault_cb()); /* Elapsed-time handling survives systick wrap. */
    assert(!raik060_vesc_start(&conf));
    puts("RAIK060 startup, failed thread, valid sample, staleness and CRC fault tests passed");
    return 0;
}
