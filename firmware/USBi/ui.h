#pragma once
void ui_init(void);   /* probes and initialises the OLED; does nothing if no display answers */
void ui_poll(void);   /* call often from the main loop; redraws about 10 times a second */
