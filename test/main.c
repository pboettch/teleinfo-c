#include <teleinfo.h>

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
			printf("conso HC  %d\n", conso_hc);
		}
		hchc = v;
	} else if (strcmp(field, "HCHP") == 0) {
		uint32_t v = strtoul(value, NULL, 10);
		if (hchp != 0 && v != hchp) {
			conso_hp += v - hchp;
			conso += v - hchp;
			change = true;
			printf("conso HP  %d\n", conso_hp);
		}
		hchp = v;
	}

	if (change) {
		struct timeval now;	
			
		gettimeofday(&now, NULL);

		double d = now.tv_sec - last.tv_sec + (now.tv_usec - last.tv_usec) / 1e6;

		double kwh_hour = (conso - conso_prev) / d * 3600;
		
		printf("%f kWh/h %f\n", kwh_hour, d);
						
		conso_prev = conso;
		last = now;
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
