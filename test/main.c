#include <teleinfo.h>

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

static uint32_t hchc, hchp, conso;

static void callback(const char *field, const char *value, void *ctx)
{
	(void) ctx;

	if (strcmp(field, "HCHC") == 0) {
		uint32_t v = strtoul(value, NULL, 10);
		if (hchc != 0 && v != hchc)
			conso += v - hchc;
		hchc = v;
	} else if (strcmp(field, "HCHP") == 0) {
		uint32_t v = strtoul(value, NULL, 10);
		if (hchp != 0 && v != hchp)
			conso += v - hchp;
		hchp = v;
	} else if (strcmp(field, "PAPP") == 0) {
		uint32_t v = strtoul(value, NULL, 10);
		printf("current %d\n", v);
	}



	printf("conso %d\n", conso);
}

int main(void)
{
	struct teleinfo_context ctx;

	teleinfo_init(&ctx, callback, NULL);

	char b[10];

	while (1) {
		ssize_t ret = read(0, b, sizeof(b));
		if (ret <= 0) {
			break;
		}

		teleinfo_consume(&ctx, b, ret);
	}

	return 0;
}
