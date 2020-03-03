typedef enum _BOOL {TRUE=1,FALSE=0} BOOL;

extern unsigned char USB_recived_buffer[32];
extern unsigned char USB_recived_buffer_size;
extern BOOL CDC_initialized_OK;
extern unsigned int tick;

void callback_USB_RXC(void);
void USB_send_buffer(unsigned int len, unsigned char *ByteToSend);
void send_msg_to_console(unsigned int len, unsigned char __flash *flash_buffer);
void START_PLL_and_USB_module(void);