#include "main.h"
#include "user_flash.h"
#include "string.h"
#include "ms_timers.h"
#include "ymodem.h"
#include "usart.h"
#include "stdbool.h"
#include "iwdg.h"
#include "stdlib.h"

//static uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
static uint8_t packet_data[PACKET_DATA_INDEX + PACKET_1K_SIZE + PACKET_TRAILER_SIZE];

#if YMODEM_RECEIVE > 0
static int32_t packet_length;
static uint8_t file_name[FILE_NAME_LENGTH];
static uint8_t file_size[FILE_SIZE_LENGTH];

/*******************************************************************************
 *
 *******************************************************************************/
static bool get_char(uint8_t *c, uint32_t timeout)
{
	static uint8_t tmr_char = NO_TIMER;

	if (tmr_char == NO_TIMER)
		tmr_char = timer_get();
	if (tmr_char != NO_TIMER)
		timer_start(tmr_char, timeout, NULL);

	while (!timer_elapsed(tmr_char))
	{
		if (ym_getc(c))
			return (true);
	}
	debug_msg("#get ch timeout\r\n");
	return (false);
}

/*******************************************************************************
 *
 *******************************************************************************/
static int32_t receive_packet(uint8_t *data, int32_t *length)
{
	uint8_t c;
	uint32_t i, packet_size;

	*length = 0;

	if (!get_char(&c, PACKET_TIMEOUT))
	{
		debug_msg("#Timeout -1\r\n");
		return (-1); // Timeout
	}

	switch (c)
	{
	case SOH:
		packet_size = PACKET_SIZE;
		debug_msg("#SOH\r\n");
		break;

	case STX:
		debug_msg("#STX\r\n");
		packet_size = PACKET_1K_SIZE;
		break;

	case EOT:
		debug_msg("#EOT\r\n");
		return (0); // Normal exit

	case CCAN:
		debug_msg("#CCAN\r\n");
		if (!get_char(&c, PACKET_TIMEOUT))
			return (-1); // Timeout

		if (c == CCAN)
		{
			debug_msg("#c=CCAN\r\n");
			*length = -1;
			return (0); // Normal exit
		}
		break;
	case ABORT:
		debug_msg("#ABORT\r\n");
		return (1); // Aborted by sender

	default:
		debug_msg("#ERR\r\n");
		return (-1); // Packet error
	}

	*data = c;

	for (i = 1; i < (packet_size + PACKET_OVERHEAD); i++)
	{
		if (!get_char(data + i, PACKET_TIMEOUT))
		{
			debug_msg("#Timeout data %d\r\n", i);
			return (-1); // Timeout
		}
	}

	if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xFF) & 0xFF))
	{
		debug_msg("#Packet error\r\n");
		return (-1); // Packet error
	}
	*length = packet_size;

	return (0); // Normal exit
}

/*******************************************************************************
 *
 *******************************************************************************/
int32_t ymodem_receive(uint32_t address, uint32_t size, bool (*callback)(void *p_args))
{
	bool session_begin, session_done, file_done;
	uint32_t i, errors, packets_received;
	uint8_t *p;
	ym_data_t ym_data;

	ym_data.addr = address;
	ym_data.size = address + size;
	ym_data.p_buf = &packet_data[PACKET_HEADER];

	ym_putc(CRC16);

	for (session_begin = false, session_done = false, errors = 0;;)
	{
		for (file_done = false, packets_received = 0;;)
		{
			wdt_clr();
			switch (receive_packet(packet_data, &packet_length))
			{

			case 0:
				errors = 0;
				switch (packet_length)
				{
				case -1: // Abort by sender
					debug_msg("#ACK -1\r\n");
					ym_putc(ACK);
					return (0);

				case 0: // End of transmission
					debug_msg("#ACK 0\r\n");
					ym_putc(ACK);
					file_done = true;
					break;

				default: // Normal packet
					if ((packet_data[PACKET_SEQNO_INDEX] & 0xFF) != (packets_received & 0xFF))
					{
						debug_msg("# %02X %02X %02X\r\n", packet_data[0], packet_data[1], packet_data[2]);
						if (((packet_data[PACKET_SEQNO_INDEX] & 0xFF) == 0) && (packets_received & 0xFF == 0))
							ym_putc(ACK); // NAK Sequence error, try again
						if ((packet_data[PACKET_SEQNO_INDEX] & 0xFF) > 0)
							ym_putc(ACK); // NAK Sequence error, try again

					}
					else
					{
						debug_msg("#%X\r\n", packet_data[PACKET_HEADER]);
						if (packets_received == 0)
						{
							if (packet_data[PACKET_HEADER] != 0)
							{
								// Copy the file name
								for (i = 0, p = packet_data + PACKET_HEADER; (*p != 0) && (i < FILE_NAME_LENGTH);)
								{
									file_name[i++] = *p++;
								}
								file_name[i++] = '\0';
								debug_msg("#File name: %s\r\n", file_name);
								// Copy the file size
								for (i = 0, p++; (*p != ' ') && (i < FILE_SIZE_LENGTH);)
								{
									file_size[i++] = *p++;

								}
								file_size[i++] = '\0';
								size = atoi(file_size);
								debug_msg("#File size %d\r\n", size);

								/* Test the size of the image to be sent */
								/* Image size is greater than reserved space for application */
								if (size > APP_SIZE)
								{
									debug_msg("#Abort File size\r\n");
									ym_putc(CCAN);
									ym_putc(CCAN);
									return (-1);
								}

								ym_putc(ACK);
								ym_putc(CRC16);
								debug_msg("#ACK+CRC16\r\n");
							}
							else
							{
								ym_putc(ACK);
								ym_putc(ACK);
								debug_msg("#ACK+ACK\r\n");
								file_done = true;
								session_done = true;
								break;
							}
						}
						else
						{
							// Write received data
							if (ym_data.addr > ym_data.size)
							{
								// Size failure
								ym_putc(CCAN);
								ym_putc(CCAN);
								debug_msg("#Size failure\r\n");
								return (-1);
							}

							ym_data.len = packet_length;
							if (callback((void *) &ym_data))
							{
								// Write success
								ym_data.addr += packet_length;
							}
							else
							{
								// Write failure
								debug_msg("#Write failure\r\n");
								ym_putc(CCAN);
								ym_putc(CCAN);
								return (-1);
							}
							ym_putc(ACK);
							debug_msg("ACK1\r\n");
						}

						packets_received++;
						session_begin = true;
					}
				}
				break;

			case 1:
				ym_putc(CCAN);
				ym_putc(CCAN);
				debug_msg("#-3\r\n");
				return (-3);

			default:
				if (session_begin)
				{
					errors++;
					if ((errors % 20) == 0)
					{
						ym_putc(NAK);
						ym_putc(CRC16);
						debug_msg("#Begin\r\n");
					}
				}

				if (errors > MAX_ERRORS)
				{
					ym_putc(CCAN);
					ym_putc(CCAN);
					debug_msg("#Max error=%d\r\n", errors);
					return (0);
				}

				if (packets_received == 0)
				{
					debug_msg("#p=0\r\n");
					ym_putc(CRC16);
				}
				break;
			} // end of packet switch

			if (file_done)
			{
				debug_msg("#Done\r\n");
				//session_done = true; // cheat TeraTerm
				break;
			}
		}
		if (session_done)
		{
			debug_msg("#Done1\r\n");
			break;
		}
	}
	debug_msg("#0\r\n");
	return (0);
}

#endif
#if YMODEM_SEND > 0

static uint8_t tmr = NO_TIMER;
static uint8_t state = 0;

// user defined function
bool flash_open(const char *filename)
{
	return 1;
}

// user defined function
bool flash_read(uint8_t *p_data, uint32_t len)
{
	static uint8_t *offset = (uint8_t *) 0x0800A000;
	memcpy(p_data, offset, len);
	offset += len;

	return 1;
}

// user defined function
bool flash_close(void)
{
	return 1;
}

static uint16_t crc16(const uint8_t *p_data, uint16_t size)
{
	uint16_t crc = 0;
	uint8_t i;

	while (size--)
	{
		crc = crc ^ ((uint16_t) *p_data++ << 8);
		for (i = 0; i < 8; i++)
		{
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc = (crc << 1);
		}
	}

	return (crc);
}

static uint16_t prepare_intial_packet(uint8_t *p_packet, ymt_data_t ym_data)
{
	uint8_t i, j = PACKET_DATA_INDEX;
	char str[FILE_SIZE_LENGTH];
	uint16_t crc;

	if (ym_data.f_open)
	{
		if (!ym_data.f_open(ym_data.filename))
			return (0);
	}

	p_packet[PACKET_START_INDEX] = SOH;
	p_packet[PACKET_NUMBER_INDEX] = 0x00;
	p_packet[PACKET_CNUMBER_INDEX] = 0xFF;

	for (i = 0; (ym_data.filename[i] != '\0') && (i < FILE_NAME_LENGTH);)
		p_packet[j++] = ym_data.filename[i++];
	p_packet[j++] = '\0';

	itoa(ym_data.filesize, str, 10);

	for (i = 0; (str[i] != '\0') && (i < FILE_SIZE_LENGTH);)
		p_packet[j++] = str[i++];
	p_packet[j++] = '\0';

	for (; j < (PACKET_SIZE + PACKET_DATA_INDEX);)
		p_packet[j++] = '\0';

	crc = crc16(&p_packet[PACKET_DATA_INDEX], PACKET_SIZE);
	p_packet[j++] = (crc >> 8);
	p_packet[j++] = (crc & 0xFF);

	return (PACKET_SIZE);
}

static uint16_t prepare_packet(uint8_t *p_packet, uint8_t pkt_nr, uint32_t size, ymt_data_t ym_data)
{
	uint16_t pkt_size, blk_size;
	uint16_t crc;
	uint16_t i;

	pkt_size = (size >= PACKET_1K_SIZE) ? PACKET_1K_SIZE : PACKET_SIZE;
	blk_size = (size < pkt_size) ? size : pkt_size;

	p_packet[PACKET_START_INDEX] = (pkt_size == PACKET_1K_SIZE) ? STX : SOH;
	p_packet[PACKET_NUMBER_INDEX] = pkt_nr;
	p_packet[PACKET_CNUMBER_INDEX] = (uint8_t) (~pkt_nr);

	if (size)
	{
		if (ym_data.f_read)
		{
			if (!ym_data.f_read(&p_packet[PACKET_DATA_INDEX], blk_size))
				return (0);
		}

		for (i = (blk_size + PACKET_DATA_INDEX); i < (pkt_size + PACKET_DATA_INDEX); i++)
			p_packet[i] = 0x1A; // EOF
	}
	else
	{
		if (ym_data.f_close)
		{
			if (!ym_data.f_close())
				return (0);
		}

		for (i = PACKET_DATA_INDEX; i < (pkt_size + PACKET_DATA_INDEX); i++)
			p_packet[i] = '\0';
	}

	crc = crc16(&p_packet[PACKET_DATA_INDEX], pkt_size);
	p_packet[i++] = (crc >> 8);
	p_packet[i++] = (crc & 0xFF);

	return (pkt_size);
}

com_err_t ymodem_transmit(ymt_data_t ym_data)
{
	uint8_t c;
	uint8_t retries;
	uint8_t blk_number = 1;
	uint16_t pkt_size;
	bool done = false;
	com_err_t err = COM_OK;

	// Allocate a timer.
	if (tmr == NO_TIMER)
		tmr = timer_get();
	if (tmr == NO_TIMER)
		return (COM_ERROR);

	while ((err == COM_OK) && !done)
	{
		wdt_clear();

		switch (state)
		{
		case 0:
			retries = 1;
			timer_start(tmr, 0);
			state++;
			break;

		case 1:
			if (!timer_elapsed(tmr))
			{
				if (ym_getc(&c))
				{
					debug_msg("case 1: %02X\r\n", c);
					if (c == CRC16)
					{
						retries = 3;
						timer_start(tmr, 0);
						state++;
					}
				}
			}
			else if (retries)
			{
				debug_msg("case 1 retry: %d\r\n", retries);
				timer_start(tmr, 30000);
				retries--;
			}
			else
			{
				err = COM_TIMEOUT;
				state = 0;
			}
			break;

		case 2:
			if (!timer_elapsed(tmr))
			{
				if (ym_getc(&c))
				{
					debug_msg("case 2: %02X\r\n", c);
					if (c == ACK)
					{
						retries = 3;
						timer_start(tmr, 0);
						state++;
					}
					else if (c == CA)
					{
						err = COM_ABORT;
						state = 0;
					}
					else
					{
						timer_start(tmr, 0);
					}
				}
			}
			else if (retries)
			{
				debug_msg("case 2 retry: %d\r\n", retries);
				pkt_size = prepare_intial_packet(packet_data, ym_data);
				ym_putsn(&packet_data[PACKET_START_INDEX], PACKET_HEADER_SIZE + pkt_size + PACKET_TRAILER_SIZE);

				timer_start(tmr, 3000);
				retries--;
			}
			else
			{
				err = COM_TIMEOUT;
				state = 0;
			}
			break;

		case 3:
			if (!timer_elapsed(tmr))
			{
				if (ym_getc(&c))
				{
					debug_msg("case 3: %02X\r\n", c);
					if (c == CRC16)
					{
						retries = 3;
						timer_start(tmr, 0);
						state++;
					}
				}
			}
			else if (retries)
			{
				debug_msg("case 3 retry: %d\r\n", retries);
				timer_start(tmr, 1000);
				retries--;
			}
			else
			{
				err = COM_TIMEOUT;
				state = 0;
			}
			break;

		case 4:
			if (!timer_elapsed(tmr))
			{
				if (ym_getc(&c))
				{
					debug_msg("case 4: %02X\r\n", c);
					if (c == ACK)
					{
						if (ym_data.filesize >= pkt_size)
							ym_data.filesize -= pkt_size;
						else
							ym_data.filesize = 0;

						if (ym_data.filesize)
						{
							blk_number++;

							retries = 3;
							timer_start(tmr, 0);
						}
						else
						{
							retries = 3;
							timer_start(tmr, 0);
							state++;
						}
					}
					else if (c == CA)
					{
						err = COM_ABORT;
						state = 0;
					}
					else
					{
						timer_start(tmr, 0);
					}
				}
			}
			else if (retries)
			{
				debug_msg("case 4 block: %d, retry: %d\r\n", blk_number, retries);
				pkt_size = prepare_packet(packet_data, blk_number, ym_data.filesize, ym_data);
				ym_putsn(&packet_data[PACKET_START_INDEX], PACKET_HEADER_SIZE + pkt_size + PACKET_TRAILER_SIZE);

				timer_start(tmr, 10000);
				retries--;
			}
			else
			{
				err = COM_TIMEOUT;
				state = 0;
			}
			break;

		case 5:
			if (!timer_elapsed(tmr))
			{
				if (ym_getc(&c))
				{
					debug_msg("case 5: %02X\r\n", c);
					if (c == ACK)
					{
						retries = 3;
						timer_start(tmr, 0);
						state++;
					}
					else if (c == CA)
					{
						err = COM_ABORT;
						state = 0;
					}
					else
					{
						timer_start(tmr, 0);
					}
				}
			}
			else if (retries)
			{
				debug_msg("case 5 retry: %d\r\n", retries);
				ym_putc(EOT);

				timer_start(tmr, 1000);
				retries--;
			}
			else
			{
				err = COM_TIMEOUT;
				state = 0;
			}
			break;

		case 6:
			if (!timer_elapsed(tmr))
			{
				if (ym_getc(&c))
				{
					debug_msg("case 6: %02X\r\n", c);
					if (c == CRC16)
					{
						retries = 3;
						timer_start(tmr, 0);
						state++;
					}
				}
			}
			else if (retries)
			{
				debug_msg("case 6 retry: %d\r\n", retries);
				timer_start(tmr, 1000);
				retries--;
			}
			else
			{
				err = COM_TIMEOUT;
				state = 0;
			}
			break;

		case 7:
			if (!timer_elapsed(tmr))
			{
				if (ym_getc(&c))
				{
					debug_msg("case 7: %02X\r\n", c);
					if (c == ACK)
					{
						done = true;
						timer_start(tmr, 0);
						state = 0;
					}
					else if (c == CA)
					{
						err = COM_ABORT;
						state = 0;
					}
					else
					{
						timer_start(tmr, 0);
					}
				}
			}
			else if (retries)
			{
				debug_msg("case 7 retry: %d\r\n", retries);
				pkt_size = prepare_packet(packet_data, 0, ym_data.filesize, ym_data);
				ym_putsn(&packet_data[PACKET_START_INDEX], PACKET_HEADER_SIZE + pkt_size + PACKET_TRAILER_SIZE);

				timer_start(tmr, 3000);
				retries--;
			}
			else
			{
				err = COM_TIMEOUT;
				state = 0;
			}
			break;
		}
	}

	return (err);
}

#endif
