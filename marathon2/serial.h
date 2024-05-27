#include <Serial.h>

boolean is_serial_port_open(void);
boolean is_serial_port_in_use(SPortSel port);
boolean is_appletalk_active(void);

OSErr allocate_serial_port(void);
OSErr close_serial_port(void);
OSErr configure_serial_port(SPortSel port, short configure);

OSErr send_serial_bytes(byte *buffer, short count);
OSErr receive_serial_byte(boolean *received, byte *buffer);

long get_serial_buffer_size(void);
