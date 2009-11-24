#include <stdio.h>
#include <errno.h>
#if 1 //def USE_SPANDSP_CALLERID
#include <stdlib.h>	/* ssize_t */
#include <inttypes.h>
#include <assert.h>
#include "spandsp/telephony.h"
#include "spandsp/logging.h"
#include "spandsp/complex.h"
#include "spandsp/dds.h"
#include "spandsp/power_meter.h"
#include "spandsp/async.h"
#include "spandsp/crc.h"
#include "spandsp/fsk.h"
#include "spandsp/tone_detect.h"
#include "spandsp/tone_generate.h"
#include "spandsp/super_tone_rx.h"
#include "spandsp/queue.h"
#include "spandsp/dtmf.h"
#include "spandsp/adsi.h"
#endif

#define BLOCK_SIZE 160	/* 20ms */

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

int main(int argc, char *argv[])
{
	static FILE *fh;
	short linear[BLOCK_SIZE];
	adsi_rx_state_t adsi;

	if ((fh = fopen(argv[1], "r"))<0) {
		fprintf(stderr, "Could not open %s for reading: %s\n", 
				argv[1], strerror(errno));
		exit(1);
	}

	adsi_rx_init(&adsi, ADSI_STANDARD_CLASS, cid_put_msg, NULL);

	for (;;) {
		if (fread((char *)linear, 2, BLOCK_SIZE, fh)!= BLOCK_SIZE)
			break;

		fsk_rx(&adsi.fskrx, linear, BLOCK_SIZE);
	}

	printf("Done\n");
	fclose(fh);
}
