/* ESPsoup radar link: buffers a 60 GHz radar module (MR60BHA2 / HLK-LD6002,
 * TinyFrame protocol) on UART1 and exposes it through two serial commands.
 * The radar bytes are passed through untouched; decoding happens on the host. */
#pragma once
#include <stdbool.h>

void radar_link_init(void);
/* Handles RADAR?, RADARTX <hex>, RADARINFO?. Returns true if the line was consumed. */
bool radar_link_command(const char *line);
