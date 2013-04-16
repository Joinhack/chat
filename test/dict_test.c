#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <jmalloc.h>
#include <dict.h>
#include <cstr.h>

#include <sys/time.h>


unsigned int hash(const void *key) {
	return dict_generic_hash((char*)key, strlen((char*)key));
}

int compare(const void *k1, const void *k2) {
	return strcasecmp((char*)k1, (char*)k2) == 0;
}

void* cpy(const void *k1) {
	char *p = jmalloc(strlen((char*)k1));
	memset(p, 0, sizeof(p));
	strcpy(p, k1);
	return p;
}

void destroy(void *k1) {
	jfree(k1);
}

dict_opts opts = {
	.hash = hash,
	cpy,
	cpy,
	compare,
	destroy,
	destroy,
};

int main() {
	size_t i = 0, m, max=100000;
	char buf[128];
	struct timeval beg, end;
	long sec ;
	dict *d = dict_create(&opts);
	for(m = 0; m < 3; m++) {
		for(i = 0; i < max; i++) {
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), ";;%lu;;", i);
			dict_add(d, buf, buf);
		}
		printf("add %d, %d %llu\n", DICT_USED(d), DICT_CAP(d),used_mem());
		for(i = 0; i < max; i++) {
			dict_entry *entry;
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), ";;%lu;;", i);
			entry = dict_find(d, buf);
			if(strcmp((char*)entry->value, (char*)buf) != 0)
				abort();
		}
		printf("find %d, %d %llu\n", DICT_USED(d), DICT_CAP(d),used_mem());
		for(i = 0; i < max; i++) {
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), ";;%lu;;", i);
			dict_replace(d, buf, buf);
		}
		printf("prelace %d, %d %llu\n", DICT_USED(d), DICT_CAP(d),used_mem());
		for(i = 0; i < max; i++) {
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), ";;%lu;;", i);
			dict_del(d, buf);
		}
		printf("delete %d, %d %llu\n", DICT_USED(d), DICT_CAP(d),used_mem());
	}
	dict_expand(d, max*2);
	printf("%d, %d %llu\n", DICT_USED(d), DICT_CAP(d),used_mem());
	printf("%d %llu\n", DICT_CAP(d),used_mem());
	dict_destroy(d);
	printf("%llu\n", used_mem());


	d = dict_create(&opts);

	gettimeofday(&beg, NULL);
	for(i = 0; i < 1000000; i++) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%lu", i);
		dict_add(d, buf, buf);
	}
	gettimeofday(&end, NULL);
	sec = (end.tv_sec - beg.tv_sec)*1000;
	sec += (end.tv_usec - beg.tv_usec)/1000;
	printf("add msec %ld, mm: %llu\n", sec, used_mem());

	gettimeofday(&beg, NULL);
	dict_iterator *iter = dict_get_iterator(d);
	dict_entry *entry = NULL;
	i = 0;
	while((entry = dict_iterator_next(iter)) != NULL){
		i++;
	}
	dict_iterator_destroy(iter);
	gettimeofday(&end, NULL);
	sec = (end.tv_sec - beg.tv_sec)*1000;
	sec += (end.tv_usec - beg.tv_usec)/1000;
	printf("iterator msec %ld, times: %lu mm: %llu\n", sec, i, used_mem());

	gettimeofday(&beg, NULL);
	for(i = 0; i < 1000000; i++) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%lu", i);
		dict_del(d, buf);
	}
	gettimeofday(&end, NULL);
	sec = (end.tv_sec - beg.tv_sec)*1000;
	sec += (end.tv_usec - beg.tv_usec)/1000;
	printf("remove msec %ld, mm: %llu\n", sec, used_mem());

	dict_destroy(d);
	printf("%llu\r\n", used_mem());


	return 0;
}