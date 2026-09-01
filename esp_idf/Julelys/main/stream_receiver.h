#ifndef STREAM_RECEIVER_H
#define STREAM_RECEIVER_H

#include <stdint.h>

/* UDP port the manager streams frames to. */
#define STREAM_PORT 2412

/* Wire format, one packet per LED row:
 *
 *   offset  size  field
 *   0       2     frame_seq  (big endian, wraps at 65535)
 *   2       1     row        (0 .. NUMBER_OF_LINES-1)
 *   3       1     flags      (bit0 = last row of this frame)
 *   4       220   RGBW       (NUMBER_OF_LEDS_LINES * 4 bytes)
 *
 * A full 8x55 frame is 8 packets of 224 bytes, which keeps every packet
 * inside the 1500 byte MTU. Losing one packet costs a single row for a
 * single frame instead of the whole image.
 */
#define STREAM_HEADER_LEN 4
#define STREAM_FLAG_END_OF_FRAME 0x01

void startupStreamTask();

#endif /* STREAM_RECEIVER_H */
