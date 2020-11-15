#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

u_int8_t array[10*4096];

volatile uint64_t counter = 0;

static inline void flush(void *addr) {
	asm volatile ("DC CIVAC, %[ad]" : : [ad] "r" (addr));
	asm volatile("DSB SY");
}


void *inc_counter(void *a) {
	while (1) {
		counter++;
		asm volatile ("DMB SY");
	}
}
static u_int64_t timed_read(volatile uint8_t *addr) {
	u_int64_t ns = counter;

	asm volatile (
		"DSB SY\n"
		"LDR X5, [%[ad]]\n"
		"DSB SY\n"
		: : [ad] "r" (addr) : "x5");

	return counter - ns;
}


int main(int argc, const char**argv)
{
  int junk=0;
  register u_int64_t time1, time2;
  volatile u_int8_t*addr;
  int i;// Initialize the array
  for(i=0; i<10; i++) array[i*4096]=i;
  // FLUSH the array from the CPU cache
  pthread_t inc_counter_thread;
	if (pthread_create(&inc_counter_thread, NULL, inc_counter, NULL)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}
    // let the bullets fly a bit ....
	while (counter < 10000000);
	asm volatile ("DSB SY");

  for(i=0; i<10; i++){
    flush(&array[i*4096]);
  }
  // Access some of the array items
  array[3*4096] = 100;
  array[7*4096] = 200;
  array[5*4096] = 300;

  for(i=0; i<10; i++) {
    addr = &array[i*4096];
    //time1 = __rdtscp(&junk);
    //junk =*addr;
    //time2 = __rdtscp(&junk) - time1;
    time1 = timed_read(addr);
    printf("Access time for array[%d*4096]: %d CPU cycles\n",i, (int)time1);
  }
  return 0;
}
