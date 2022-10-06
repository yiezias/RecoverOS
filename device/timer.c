
#include "timer.h"
#include "../kernel/debug.h"
#include "../kernel/intr.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/print.h"
#include "../thread/thread.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define CONTER0_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTER0_PORT 0x40
#define CONTER0_NO 0
#define CONTER_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL_PORT 0x43

#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCY)


size_t ticks = 0;

static void freqc_set(uint8_t CounterPort, uint8_t CounterNo, uint8_t rwl,
		      uint8_t CounterMode, uint8_t CounterValue) {
	outb(PIT_CONTROL_PORT,
	     (uint8_t)(CounterNo << 6 | rwl << 4 | CounterMode << 1));
	outb(CounterPort, (uint8_t)CounterValue);
	outb(CounterPort, (uint8_t)CounterValue >> 8);
}



static void ticks_to_sleep(size_t sleep_ticks) {
	size_t start_tick = ticks;
	while (ticks - start_tick < sleep_ticks) {
		thread_yield();
	}
}

static void intr_time_hdl(void) {
	ASSERT(current->stack_magic == STACK_MAGIC);
	ASSERT(current->istack_magic == STACK_MAGIC);
	++ticks;
	if (current->ticks == 0) {
		schedule();
	} else {
		current->ticks--;
	}
}



void mtime_sleep(size_t m_seconds) {
	size_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);
	ASSERT(sleep_ticks > 0);
	ticks_to_sleep(sleep_ticks);
}



void timer_init() {
	put_str("timer_init: start\n");

	freqc_set(CONTER0_PORT, CONTER0_NO, READ_WRITE_LATCH, CONTER_MODE,
		  (uint8_t)(CONTER0_VALUE));
	register_intr_hdl(0x20, intr_time_hdl);
	put_str("timer_init: done\n");
}

size_t sys_getticks(void) {
	ASSERT(intr_off == get_intr_stat());
	return ticks;
}
