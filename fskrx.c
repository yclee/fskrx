#include <stdio.h>
#include <errno.h>
#include <stdlib.h>	/* ssize_t, exit() */

/* Uncomment if analysis of datalink is not required. */
/* #define NO_DATALINK_ANALYSIS */

/* Uncomment if index is not required. Will be much faster. */
/* #define NO_INDEX */

#if 1 //def USE_SPANDSP_CALLERID
#include <inttypes.h>
#ifdef NO_DATALINK_ANALYSIS
#include <assert.h>
#include "spandsp/telephony.h"
#include "spandsp/logging.h"
#include "spandsp/complex.h"
#include "spandsp/dds.h"
#endif
#include "spandsp/power_meter.h"	/* fsk.h: power_meter_t */
#include "spandsp/async.h"		/* fsk.h: get_bit_func_t */
#ifdef NO_DATALINK_ANALYSIS
#include "spandsp/crc.h"
#endif
#include "spandsp/fsk.h"
#ifdef NO_DATALINK_ANALYSIS
#include "spandsp/tone_detect.h"
#include "spandsp/tone_generate.h"
#include "spandsp/super_tone_rx.h"
#include "spandsp/queue.h"
#include "spandsp/dtmf.h"
#include "spandsp/adsi.h"
#endif
#endif

#ifdef NO_INDEX
#define BLOCK_SIZE 160	/* 20ms */
#else
#define BLOCK_SIZE 1 	/* One sample by one sample to show index */
#endif

static void cid_put_msg(void *user_data, const uint8_t *msg, int len)
{
	unsigned int i;
	unsigned int type;
	unsigned int length;

	printf("msg:");
	for (i=0; i<len; i++) {
		switch(i) {
			case 0:
				type = msg[i];
				printf(" type=0x%x", msg[i]);
				break;
			case 1:
				length = msg[i];
				printf(" len=%d", length);
				break;
			default:
				if (length>=8) {
					unsigned int j;
					printf(" time=");
					for (j=0; j<8; j++)
						printf("%c", msg[i++]);
					length-=8;
					if (length+i <= len) {
						printf(" phone#=");
						for (j=0; j<length; j++)
							printf("%c", msg[i++]);
						length = 0;
					}
				} else
					printf(" 0x%x", msg[i]);
		}
	}
	printf("\n");
}

#ifndef NO_DATALINK_ANALYSIS
typedef struct
{
    int consecutive_ones;
    int bit_pos;
    int in_progress;
    uint8_t msg[256];
    int msg_len;
} my_cid_state_t;

static void adsi_rx_put_bit(void *user_data, int bit)
{
    my_cid_state_t *s;
    int i;
    int sum;

    s = (my_cid_state_t *) user_data;
    if (bit < 0)
    {
        /* Special conditions */
        switch (bit)
        {
        case PUTBIT_CARRIER_UP:
            printf("Carrier up.\n");
            s->consecutive_ones = 0;
            s->bit_pos = 0;
            s->in_progress = 0;
            s->msg_len = 0;
            break;
        case PUTBIT_CARRIER_DOWN:
            printf("Carrier down.\n");
            break;
        default:
            printf("Unexpected special put bit value - %d!\n", bit);
            break;
        }
        return;
    }
    bit &= 1;
    if (s->bit_pos == 0)
    {
        if (bit == 0)
        {
            /* Start bit */
            s->bit_pos++;
            if (s->consecutive_ones > 10)
            {
                /* This is a line idle condition, which means we should
                   restart message acquisition */
                s->msg_len = 0;
		printf("=======Restart acquisition=======\n");
            }
            s->consecutive_ones = 0;
        }
        else
        {
            s->consecutive_ones++;
        }
    }
    else if (s->bit_pos <= 8)
    {
        s->in_progress >>= 1;
        if (bit)
            s->in_progress |= 0x80;
        s->bit_pos++;
    }
    else
    {
        /* Stop bit */
        if (bit)
        {
            if (s->msg_len < 256)
            {
                    s->msg[s->msg_len++] = (uint8_t) s->in_progress;
		    printf("Byte 0x%x\n", s->in_progress);
                    if (s->msg_len >= 3  &&  s->msg_len == (s->msg[1] + 3))
                    {
                        /* Test the checksum */
                        sum = 0;
                        for (i = 0;  i < s->msg_len - 1;  i++)
                            sum += s->msg[i];
                        if ((-sum & 0xFF) == s->msg[i])
                            cid_put_msg(NULL, s->msg, s->msg_len - 1);
                        else
                            printf("Sumcheck failed\n");
                        s->msg_len = 0;
                    }
            }
        }
        else
        {
		printf("Stop-bit error, Byte 0x%x\n", s->in_progress);
        }
        s->bit_pos = 0;
        s->in_progress = 0;
    }
}
#endif

int main(int argc, char *argv[])
{
	static FILE *fh;
	short linear[BLOCK_SIZE];
#ifndef NO_INDEX
	unsigned int index = 0;
#endif
#ifdef NO_DATALINK_ANALYSIS
	adsi_rx_state_t adsi;
#else
	fsk_rx_state_t fskrx;
	fsk_spec_t fsk_bell202 = {
		"Bell202",
		2200,
		1200,
		-14,
		-30,
		1200
	};
	my_cid_state_t cid;
#endif

	if ((fh = fopen(argv[1], "r"))<0) {
		fprintf(stderr, "Could not open %s for reading: %s\n", 
				argv[1], strerror(errno));
		exit(1);
	}

#ifdef NO_DATALINK_ANALYSIS
	adsi_rx_init(&adsi, ADSI_STANDARD_CLASS, cid_put_msg, NULL);
#else
        fsk_rx_init(&fskrx, &fsk_bell202, 0, adsi_rx_put_bit, &cid);
#endif

	for (;;) {
		if (fread((char *)linear, 2, BLOCK_SIZE, fh)!= BLOCK_SIZE)
			break;

#ifndef NO_INDEX
		index += BLOCK_SIZE;
		printf("\r%d:", index);
#endif
#ifdef NO_DATALINK_ANALYSIS
		fsk_rx(&adsi.fskrx, linear, BLOCK_SIZE);
#else
		fsk_rx(&fskrx, linear, BLOCK_SIZE);
#endif
	}

	printf("Done\n");
	fclose(fh);
}
