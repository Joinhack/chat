#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <common.h>

int main() {
	long long l;
	long lv;
	int v;
	char buf[1024];

	char *p = "-101212125";
	str2ll(p, strlen(p), &l);
	printf("%lld\n", l);


	p = "1234567890";
	str2ll(p, strlen(p), &l);
	printf("%lld\n", l);

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%lld", LLONG_MAX);
	str2ll(buf, strlen(buf), &l);
	memset(buf, 0, sizeof(buf));
	ll2str(l, buf, sizeof(buf));
	printf("%s, %lld\n", buf, l);

	printf("%lld\n", LLONG_MIN);
	p = "-9223372036854775808";
	v = str2ll(p, strlen(p), &l);
	memset(buf, 0, sizeof(buf));
	ll2str(l, buf, sizeof(buf));
	printf("%d, %s, %lld\n", v, p, l);

	p = "-9223372036854775809";
	v = str2ll(p, strlen(p), &l);
	printf("%d, %s, %lld\n", v, p, l);

	p = "9223372036854775808";
	v = str2ll(p, strlen(p), &l);
	printf("%d, %s, %lld\n", v, p, l);

	p = "0001";
	v = str2ll(p, strlen(p), &l);
	printf("%d, %s, %lld\n", v, p, l);

	v = ll2str(1234567890L, buf, 9);
	buf[v] = 0;
	printf("%d, %s\n", v, buf);	

	v = ll2str(-101212125L, buf, 9);
	buf[v] = 0;
	printf("%d, %s\n", v, buf);	
	return 0;
}
