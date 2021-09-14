#include <kern/time.h>
#include <inc/assert.h>

static unsigned int ticks;

void
time_init(void)
{
	ticks = 0;
}

// This should be called once per timer interrupt.  A timer interrupt
// fires every 10 ms.
// 每10毫秒产生一次timer interrupt,每次timer interrupt之后调用time_tick()
// 每次调用ticks++,所以ticks * 10为当前时间
void
time_tick(void)
{
	ticks++;
	if (ticks * 10 < ticks)
		panic("time_tick: time overflowed");
}

unsigned int
time_msec(void)
{
	return ticks * 10;
}
