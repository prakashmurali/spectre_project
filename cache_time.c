#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

u_int8_t array[256*4096];

volatile uint64_t counter = 0;

static inline void flush(void *address) {
	asm volatile ("DC CIVAC, %0" : : "r" (address)); //Cache line clear and invalidate based on virtual address
	asm volatile("DSB SY"); // Barrier, forces the cache flush to complete before the barrier
}

void *increment_counter(void *a) {
	while (1) {
		counter++;
		asm volatile ("DMB SY"); // Memory barrier, forces the increment to complete
	}
}

static u_int64_t timed_read(volatile uint8_t *address) {
	u_int64_t ns = counter;
	asm volatile (
		"DSB SY\n" //memory barrier
		"LDR X0, [%0]\n" // load into register x0, the value in [address]
		"DSB SY\n" //memory barrier
		: : "r" (address) : "x0"); // x0 is added to clobber list since its not an explicit output value
	return counter - ns;
}


void print_stats(u_int64_t data[]) {
    float sum = 0.0, mean, SD = 0.0;
    int i;
    for (i = 0; i < 10000; ++i) {
        sum += data[i];
    }
    mean = sum / 10000;
    for (i = 0; i < 10000; ++i)
        SD += (data[i] - mean)*(data[i] - mean);
   	SD = sqrt(SD / 10000);
		printf("Mean %f SD %f\n", mean, SD);
}


int main(int argc, const char**argv)
{
  int junk=0;
  register u_int64_t time1, time2;
  volatile u_int8_t*addr;
  int i;// Initialize the array

	pthread_t counter_thread;
  pthread_create(&counter_thread, NULL, increment_counter, NULL);
  while (counter < 10); //sleep
  asm volatile ("DSB SY");

	u_int64_t miss_times[10000];
	u_int64_t hit_times[10000];

	int trials;
	for(trials = 0; trials < 10000; trials++){
		int load_idx;
		while(1){
			load_idx = rand() % 256;
			if(load_idx == 100)
				continue;
			else
				break;
		}

		for(i=0; i<256; i++) array[i*4096]=i;
	  for(i=0; i<256; i++){
	    flush(&array[i*4096]);
	  }

		array[load_idx*4096] = 1;
		asm volatile ("DMB SY");

		addr = &array[load_idx*4096];
		time1 = timed_read(addr);
		addr = &array[100*4096];
		time2 = timed_read(addr);
    //printf("Access time for array[%d*4096]: %d CPU cycles\n",load_idx, (int)time1);
		//printf("Access time for array[%d*4096]: %d CPU cycles\n",100, (int)time2);

		hit_times[trials] = time1;
		miss_times[trials] = time2;
	}

  print_stats(hit_times);
	print_stats(miss_times);
  return 0;
}
