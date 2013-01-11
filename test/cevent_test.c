#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <cevent.h>

int main(int argc, char const *argv[]) {
	cevents *evts = cevents_create();
	while(1) 
		cevents_poll(evts, 10);
	destory_cevents(evts);
	return 0;
}
