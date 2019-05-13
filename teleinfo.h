// Header-only, C library for parsing and extracting teleinfo-data
// from French electricity counters
//
// Copyright (C) 2019 Patrick Boettcher <p@yai.se>
//
// SPDX-License-Identifier: LGPL-3.0
//
// Version: 1.0.0
//
// Project website: https://github.com/pboettch/teleinfo-c

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct teleinfo_context {
	enum {
		RESET,
		SYNC,
		LINE,
		CHECKSUM,
	} state;

	char line[9 + 1 + 12 + 1 + 1 + 1];
	char *pos;

	unsigned char checksum;

	unsigned errors;

	void (*cb)(const char *, const char *, void *);
	void *cb_ctx;
};

/* declarations - to make inline work (link) correclty */
void teleinfo_reset(struct teleinfo_context *ctx);
void teleinfo_init(struct teleinfo_context *ctx, void (*cb)(const char *, const char *, void *), void *cb_ctx);
bool is_control_char(char c);
void teleinfo_consume(struct teleinfo_context *ctx, const char *buf, size_t len);

/* implementation */
inline void teleinfo_reset(struct teleinfo_context *ctx)
{
	ctx->state = RESET;
}

inline void teleinfo_init(struct teleinfo_context *ctx, void (*cb)(const char *, const char *, void *), void *cb_ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	teleinfo_reset(ctx);
	ctx->cb = cb;
	ctx->cb_ctx = cb_ctx;
}

#define teleinfo_is_control_char(c) ((c) < 32)

inline void teleinfo_consume(struct teleinfo_context *ctx, const char *buf, size_t len)
{
	while (len) {
		char c = *buf;

		switch (ctx->state) {
		case RESET: // stay in RESET while no control char
			if (teleinfo_is_control_char(c))
				ctx->state = SYNC;
			break;

		case SYNC: // wait for any other char than a control char to start consuming payload
			if (teleinfo_is_control_char(c))
				break;

			ctx->state = LINE;
			ctx->pos = ctx->line;
			ctx->checksum = 0;
			/* fall through */

		case LINE:
			if (teleinfo_is_control_char(c)) { // LINE is done
				char *saveptr;

				const char *etiquette = strtok_r(ctx->line, " ", &saveptr);
				const char *data = strtok_r(NULL, " ", &saveptr);
				const char *checksum = strtok_r(NULL, " ", &saveptr);

				if (etiquette == NULL || data == NULL || checksum == NULL) {
					ctx->state = RESET;
					break;
				}
				ctx->checksum -= *checksum;
				ctx->checksum &= 0x3f;

				// info("%s %s %s %d - %d\n", etiquette, data, checksum, *checksum & 0x3f, ctx->checksum);

				if ((*checksum & 0x3f) == ctx->checksum)
					ctx->cb(etiquette, data, ctx->cb_ctx);

				// DONE
				ctx->state = SYNC;
				break;
			}
			*ctx->pos++ = c;

			if ((size_t)(ctx->pos - ctx->line) > sizeof(ctx->line)) {
				ctx->errors++;
				teleinfo_reset(ctx);
				ctx->state = RESET;
			}
			*ctx->pos = '\0';

			ctx->checksum += c;

			break;

		default:
			break;
		}
		len--;
		buf++;
	}
}
