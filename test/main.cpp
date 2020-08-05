#include <teleinfo.h>

#include <vector>

#include <cmath>

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

struct teleinfo_data {
	char ADCO[13];
	char OPTTARIF[5];
	uint8_t ISOUSC;
	uint32_t HCHC;
	uint32_t HCHP;
	uint16_t IMAX;
	uint32_t PAPP;
	char HHPHC;
	char MOTDETAT[7];
};

static uint32_t hchc, hchp, conso_hc, conso_hp, conso;
static uint32_t conso_prev;
static struct timeval last;       

struct conso_item
{
	struct timeval tv;
	uint32_t value;
};

static std::vector<conso_item> conso_list;

static double diff_time(const struct timeval &l, const struct timeval &r)
{
	return fabs(l.tv_sec - r.tv_sec + (l.tv_usec - r.tv_usec) / 1e6);
}

static void last_values(size_t count) 
{
	if (conso_list.size() > count) {
		auto l = conso_list.end() - 1,
		     r = l - count;

		double d = diff_time(l->tv, r->tv ); 
		double kwh_hour = fabs(l->value - r->value) / d * 3600;
		printf("OVER %2d: %f kWh/h %f\n", count, kwh_hour, d);
	}
}

static void callback(const char *field, const char *value, void *ctx)
{
	(void) ctx;
	bool change = false;

	if (strcmp(field, "HCHC") == 0) {
		uint32_t v = strtoul(value, NULL, 10);
		if (hchc != 0 && v != hchc) {
			conso_hc += v - hchc;
			conso += v - hchc;
			change = true;
//			printf("conso HC  %d\n", conso_hc);
		}
		hchc = v;
	} else if (strcmp(field, "HCHP") == 0) {
		uint32_t v = strtoul(value, NULL, 10);
		if (hchp != 0 && v != hchp) {
			conso_hp += v - hchp;
			conso += v - hchp;
			change = true;
//			printf("conso HP  %d\n", conso_hp);
		}
		hchp = v;
	}

	if (change) {
		struct timeval now;	
		gettimeofday(&now, NULL);

		double d = diff_time(now, last); 
		double kwh_hour = (conso - conso_prev) / d * 3600;
		
		printf("------------\n");
		printf("TOTAL  : %.3f kWh/h\n", conso / 1000.0);
		printf("INSTANT: %f kWh/h %f\n", kwh_hour, d);
						
		conso_prev = conso;
		last = now;

		conso_list.push_back({now, conso_hp + conso_hc});

		last_values(5);
		last_values(10);
		last_values(20);

		if (conso_list.size() > 100)
			conso_list.erase(conso_list.begin());
	}
}

int main(void)
{
	struct teleinfo_context ctx;

	teleinfo_init(&ctx, callback, NULL);

	char b[10];

	gettimeofday(&last, NULL);
	while (1) {
		ssize_t ret = read(0, b, sizeof(b));
		if (ret <= 0) {
			break;
		}

		teleinfo_consume(&ctx, b, ret);
	}

	return 0;
}
