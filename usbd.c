#define ENABLE_BIT_DEFINITIONS
#include <intrinsics.h>
#include "ioavr.h"
#include "usbd.h"
#include "PLL.h"

#define GET_LINE_CODING           0x21
#define SET_LINE_CODING           0x20
#define SET_CONTROL_LINE_STATE    0x22
#define SEND_BREAK                0x23
#define SEND_ENCAPSULATED_COMMAND 0x00
#define GET_ENCAPSULATED_COMMAND  0x01

#  define MSB(u16)        (((U8* )&u16)[1])
#  define LSB(u16)        (((U8* )&u16)[0])
#  define MSW(u32)        (((U16*)&u32)[1])
#  define LSW(u32)        (((U16*)&u32)[0])
#  define MSB0(u32)       (((U8* )&u32)[3])
#  define MSB1(u32)       (((U8* )&u32)[2])
#  define MSB2(u32)       (((U8* )&u32)[1])
#  define MSB3(u32)       (((U8* )&u32)[0])
#  define LSB0(u32)       MSB3(u32)
#  define LSB1(u32)       MSB2(u32)
#  define LSB2(u32)       MSB1(u32)
#  define LSB3(u32)       MSB0(u32)

#define SERIAL_ECHO_ENABLE

S_line_coding line_coding;

BOOL CDC_initialized_OK = FALSE;

#define Usb_select_endpoint(ep) (UENUM = (U8)ep)
#define read_byte_from_flash(address_short) __LPM((unsigned int)(address_short))
#define USB_Dev_Attach()  (UDCON &= ~(1<<DETACH))
#define USB_Dev_Dettach()  (UDCON |= (1<<DETACH))
#define USB_freeze_clk()  (USBCON |= (1<<FRZCLK))
#define USB_unfreeze_clk()  (USBCON &= ~(1<<FRZCLK))
#define Usb_build_ep_config0(type, dir, nyet)     ((type<<6) | (nyet<<1) | (dir))
#define Usb_build_ep_config1(size, bank     )     ((size<<4) | (bank<<2)        )
#define usb_configure_endpoint(num, type, dir, size, bank, nyet)\
                              (Usb_select_endpoint(num),        \
                               usb_config_ep(Usb_build_ep_config0(type, dir, nyet),\
                               Usb_build_ep_config1(size, bank)))
U8 __flash *pbuffer;
U8 CountDataToTransfer;

#define Usb_enable_stall_handshake()              (UECONX  |=  (1<<STALLRQ))
#define Usb_ack_receive_setup()                   (UEINTX &= ~(1<<RXSTPI))

#define CURRENT_EPSIZE            32

//! wSWAP
//! This macro swaps the U8 order in words.
//! @param x        (U16) the 16 bit word to swap
//! @return         (U16) the 16 bit word x with the 2 bytes swaped

#define wSWAP(x)  (x)
#define Usb_reset_endpoint(ep)                    (UERST   =   1 << (U8)ep, UERST  =  0)


unsigned int bmRequestType_bmRequest;
unsigned int bb;
unsigned int FifoBCount;
unsigned char tempval;
U8 usb_configuration_nb;

unsigned char USB_recived_buffer[32];
unsigned char USB_recived_buffer_size;

char USB_GENERAL_FLAG;
#define USB_connected   0

__flash S_usb_device_descriptor usb_device_descriptor =
{
    sizeof(usb_device_descriptor)
, DEVICE_DESCRIPTOR
, USB_SPECIFICATION
, DEVICE_CLASS
, DEVICE_SUB_CLASS
, DEVICE_PROTOCOL
, EP_CONTROL_LENGTH
, VENDOR_ID
, PRODUCT_ID
, RELEASE_NUMBER
, MAN_INDEX
, PROD_INDEX
, SN_INDEX
, NB_CONFIGURATION
};

// usb_user_configuration_descriptor FS
__flash S_usb_user_configuration_descriptor usb_device_config = {
 { sizeof(S_usb_configuration_descriptor)
 , CONFIGURATION_DESCRIPTOR
 //, Usb_write_word_enum_struc(sizeof(usb_conf_desc_kbd))
 , 0x0043 //TODO: Change to generic codewith sizeof
 , NB_INTERFACE
 , CONF_NB
 , CONF_INDEX
 , CONF_ATTRIBUTES
 , MAX_POWER
 }
 ,
 { sizeof(S_usb_interface_descriptor)
 , INTERFACE_DESCRIPTOR
 , INTERFACE0_NB
 , ALTERNATE0
 , NB_ENDPOINT0
 , INTERFACE0_CLASS
 , INTERFACE0_SUB_CLASS
 , INTERFACE0_PROTOCOL
 , INTERFACE0_INDEX
 }
 ,
 { 0x05, 0x24, 0x00, 0x10, 0x01, 0x05, 0x24, 0x01, 0x03, 0x01, 0x04, 0x24, 0x02, 0x06,0x05, 0x24, 0x06, 0x00, 0x01 }
 ,
 { sizeof(S_usb_endpoint_descriptor)
 , ENDPOINT_DESCRIPTOR
 , ENDPOINT_NB_3
 , EP_ATTRIBUTES_3
 , EP_SIZE_3
 , EP_INTERVAL_3
 }
 ,
 { sizeof(S_usb_interface_descriptor)
 , INTERFACE_DESCRIPTOR
 , INTERFACE1_NB
 , ALTERNATE1
 , NB_ENDPOINT1
 , INTERFACE1_CLASS
 , INTERFACE1_SUB_CLASS
 , INTERFACE1_PROTOCOL
 , INTERFACE1_INDEX
 }
 ,
 { sizeof(S_usb_endpoint_descriptor)
 , ENDPOINT_DESCRIPTOR
 , ENDPOINT_NB_1
 , EP_ATTRIBUTES_1
 , EP_SIZE_1
 , EP_INTERVAL_1
 }
 ,
 { sizeof(S_usb_endpoint_descriptor)
 , ENDPOINT_DESCRIPTOR
 , ENDPOINT_NB_2
 , EP_ATTRIBUTES_2
 , wSWAP(EP_SIZE_2)
 , EP_INTERVAL_2
 }


};

U8 usb_config_ep(U8 config0, U8 config1)
{
    UECONX |= (1 << EPEN);
    UECFG0X = config0;
    UECFG1X = (UECFG1X & (1<<ALLOC)) | config1;
    UECFG1X |= (1 << ALLOC);
    if(UESTA0X & (1 << CFGOK))
    {
      return 0x01;
    }
    else
    {
      return 0x00;
    }
}

void USB_start(void)
{
  UDIEN |= (1<<EORSTE);
  USB_unfreeze_clk();
}

void USB_init_device(void)
{
  UENUM = 0x00;
  UECONX |= (1 << EPEN);
  UECFG1X = 0x32;
  UECFG0X = 0x00;
  if(UESTA0X & (1 << CFGOK))
  {
    ;;
  }
  UEIENX = (1 << RXSTPE);
}

void usb_user_endpoint_init(U8 conf_nb)
{
  UENUM = INT_EP;
  
  usb_configure_endpoint(INT_EP,      \
                         TYPE_INTERRUPT,     \
                         DIRECTION_IN,  \
                         SIZE_32,       \
                         ONE_BANK,     \
                         NYET_ENABLED);

  usb_configure_endpoint(TX_EP,      \
                         TYPE_BULK,  \
                         DIRECTION_IN,  \
                         SIZE_32,     \
                         ONE_BANK,     \
                         NYET_ENABLED);

  usb_configure_endpoint(RX_EP,      \
                         TYPE_BULK,     \
                         DIRECTION_OUT,  \
                         SIZE_32,       \
                         TWO_BANKS,     \
                         NYET_ENABLED);

  Usb_reset_endpoint(INT_EP);
  Usb_reset_endpoint(TX_EP);
  Usb_reset_endpoint(RX_EP);
}

void USB_set_configuration(void)
{
  U8 Configuration_number;
  Configuration_number = UEDATX;
  if(Configuration_number <= NB_CONFIGURATION)
  {
    UEINTX &= ~(1 << RXSTPI);
    usb_configuration_nb = Configuration_number;
  }
  else
  {
    UECONX |= (1 << STALLRQ);
    UEINTX &= ~(1 << RXSTPI);
  }
  UEINTX &= ~(1 << TXINI);
  usb_user_endpoint_init(usb_configuration_nb);
}


#pragma vector = USB_GEN_vect //For OLD version of IAR, must be: USB_General_vect
__interrupt void usb_general_interrupt()
{
  //Изменения VBUS
  if (USBINT & (1 << VBUSTI))// && (USBCON & (1<<VBUSTE)))
   {
      USBINT = ~(1<<VBUSTI);                        //Бит VBUSTI должен быть сброшен програмно
      if (USBSTA & (1 << VBUS))                     //На пине 5V
      {
         USB_GENERAL_FLAG |= (1 << USB_connected);
         USB_start();
         USB_Dev_Attach();
      }
      else
      {
        CDC_initialized_OK = FALSE; 
        USB_Dev_Dettach();
      }
   }
   if(UDINT & (1 << EORSTI))
   {
     UDINT &= ~(1 << EORSTI);
     USB_init_device();
     
   }
  
   if(UDINT & (1 << WAKEUPI))
   {
     UDINT &= ~(1 << WAKEUPI);
   }
}


#pragma vector = USB_COM_vect//USB_Endpoint_Pipe_vect
__interrupt void usb_endpoint_interrupt()
{
  if(UEINT & (1 << EPINT0))
  {
    Usb_select_endpoint(0x00);
    bmRequestType_bmRequest  = UEDATX << 8;
    bmRequestType_bmRequest |= UEDATX;

    switch(bmRequestType_bmRequest)
    {
    case 0x8006:
      USB_get_descriptor();
      break;
  
    case 0x0005:
      USB_Set_Address();
      break;
     
    case 0x0009:
      USB_set_configuration();
      break;
      
    default:
      usb_user_read_request(bmRequestType_bmRequest);      
    break;
    }
  }
  if(UEINT & (1 << EPINT1))
  {
    Usb_select_endpoint(TX_EP);
    UEINTX &= ~(1 << TXINI);
    bb = 55;
  }
  
  if(UEINT & (1 << EPINT2))
  {
    Usb_select_endpoint(RX_EP);
    if(UEINTX & (1 << RXOUTI))
    {
      UEINTX &= ~(1 << RXOUTI);
      USB_recived_buffer_size = 0;
      unsigned int tmpatomic = (UEBCHX << 8);
      tmpatomic |= UEBCLX;
      
      FifoBCount = tmpatomic;
      while(USB_recived_buffer_size < FifoBCount)
      {
        USB_recived_buffer[USB_recived_buffer_size] = UEDATX;
        USB_recived_buffer_size++;
      }
      UEINTX &= ~(1 << FIFOCON);
      
      callback_USB_RXC();
    }
  }

  if(UEINT & (1 << EPINT3))
  {
    ;;
  }  
}

void USB_reset(void)
{
  UHWCON = 0x00;
  USBCON	= (1<<FRZCLK);
  UDIEN		= 0;
  UEIENX	= 0;
}

void USB_enable(void)
{
    UHWCON = (1<<UVREGE); //это надо если нужно включить регулятор напряжения для USB модуля
    UDIEN = ((1<<EORSTE));//Включаем нужные прерывания
    USBCON = ((1<<USBE) | (1<<OTGPADE) | (1 << VBUSTE)); //Включаем USB
    __enable_interrupt();
}

void USB_get_descriptor(void)
{
U16  wLength         ;
U8  descriptor_type ;
U8  string_type     ;
U8  dummy;
U8  nb_byte;
BOOL zlp;

   zlp             = FALSE;         /* no zero length packet */
   string_type     = UEDATX;        /* read LSB of wValue    */
   descriptor_type = UEDATX;        /* read MSB of wValue    */

   switch (descriptor_type)
   {
    case DEVICE_DESCRIPTOR:
      CountDataToTransfer = sizeof(usb_device_descriptor); //!< sizeof (usb_user_device_descriptor);
      pbuffer          = (&(usb_device_descriptor.bLength));
      break;
    case CONFIGURATION_DESCRIPTOR:
      CountDataToTransfer =  sizeof(usb_device_config); //!< sizeof (usb_user_configuration_descriptor);
      pbuffer          = (&(usb_device_config.cfg.bLength));
      break;
    default:
      Usb_enable_stall_handshake();
      Usb_ack_receive_setup();
      return;
   }

   dummy = UEDATX;                     //!< don't care of wIndex field
   dummy = UEDATX;
   LSB(wLength) = UEDATX;              //!< read wLength
   MSB(wLength) = UEDATX;
   UEINTX &= ~(1 << RXSTPI);

   if (wLength > CountDataToTransfer)
   {
      if ((CountDataToTransfer % EP_CONTROL_LENGTH) == 0) { zlp = TRUE; }
      else { zlp = FALSE; }                   //!< no need of zero length packet
   }
   else
   {
      CountDataToTransfer = (U8)wLength;         //!< send only requested number of data
   }

   while((CountDataToTransfer != 0) && (!(UEINTX & (1 << RXOUTI))))
   {
      while(!(UEINTX & (1 << TXINI)));

      nb_byte=0;
      while(CountDataToTransfer != 0)        //!< Send data until necessary
      {
         if(nb_byte++==EP_CONTROL_LENGTH) //!< Check endpoint 0 size
         {
            break;
         }

         UEDATX = (*pbuffer++);
         CountDataToTransfer --;
      }
      UEINTX &= ~(1 << TXINI);
   }


   if(UEINTX & (1 << RXOUTI)) 
   { 
     UEINTX &= ~(1 << RXOUTI); 
     return; 
   } //!< abort from Host
   
   if(zlp == TRUE)
   {
     while(!(UEINTX & (1 << TXINI)));
       UEINTX &= ~(1 << TXINI);
   }


   while(!(UEINTX & (1 << RXOUTI)));
   UEINTX &= ~(1 << RXOUTI); 
}

void USB_Set_Address(void)
{
  U8 tmp;
  tmp = (UEDATX & 0x7F);
  UDADDR = ((UDADDR & (1<<ADDEN))|tmp);     //Записываем в UDADDR адрес со сброшеным ADDEN
  UEINTX &= ~(1 << RXSTPI);                 //Шлем ACK
  UEINTX &= ~(1 << TXINI);                  //Шлем ZPL (Zero Lenght Packet)
  while(!(UEINTX & (1 << TXINI)));          //Ждем подтверждения приема ZPL
  UDADDR |= (1 << ADDEN);
}

/*
void USB_send_byte(char ByteToSend)
{
  if(CDC_initialized_OK == TRUE)
  {  
    Usb_select_endpoint(TX_EP);  
    while(!(UEINTX & (1 << TXINI)));
    
      UEINTX &= ~(1 << TXINI);
      UEDATX = ByteToSend;
      UEINTX &= ~(1 << FIFOCON); 
  }
}
*/

//Посылка строки из Flash в порт
void send_msg_to_console(unsigned int len, unsigned char __flash *flash_buffer)
{
  if(CDC_initialized_OK == TRUE)
  {
      unsigned char block_counter = 0;
      Usb_select_endpoint(TX_EP);
      
      while(!(UEINTX & (1 << TXINI)));
      UEINTX &= ~(1 << TXINI);

        for(; len >0; len--)
        {
          UEDATX = *flash_buffer;
          flash_buffer++;
        
        block_counter++;
        if(block_counter == CURRENT_EPSIZE)
        {
          UEINTX &= ~(1 << FIFOCON);
          while(!(UEINTX & (1 << TXINI)));
          UEINTX &= ~(1 << TXINI);
          block_counter = 0;
        }
      } 
      
      UEINTX &= ~(1 << FIFOCON);
  }
}

//Посылка строки из SRAM буффера в порт
void USB_send_buffer(unsigned int len, unsigned char *ByteToSend)
{
  if(CDC_initialized_OK == TRUE)
  {
      unsigned char block_counter = 0;
      Usb_select_endpoint(TX_EP);
      
      while(!(UEINTX & (1 << TXINI)));
      UEINTX &= ~(1 << TXINI);
      
      for(; len >0; len--)
      {
        UEDATX = *ByteToSend;
        ByteToSend++;
        block_counter++;
        if(block_counter == CURRENT_EPSIZE)
        {
          UEINTX &= ~(1 << FIFOCON);
          while(!(UEINTX & (1 << TXINI)));
          UEINTX &= ~(1 << TXINI);
          block_counter = 0;
        }
      }
      UEINTX &= ~(1 << FIFOCON);
  }
}

void cdc_set_control_line_state (void)
{
  UEINTX &= ~(1 << RXSTPI);           //Шлем ACK
  UEINTX &= ~(1 << TXINI);
  	while(!(UEINTX & (1 << TXINI)));
  CDC_initialized_OK =TRUE;
  Usb_select_endpoint(RX_EP);
  UEIENX = (1 << RXOUTE);  
}

void cdc_get_line_coding(void)
{
  UEINTX &= ~(1 << RXSTPI);           //Шлем ACK
  UEDATX = (LSB0(line_coding.dwDTERate));
  UEDATX = (LSB1(line_coding.dwDTERate));
  UEDATX = (LSB2(line_coding.dwDTERate));
  UEDATX = (LSB3(line_coding.dwDTERate));
  UEDATX = (line_coding.bCharFormat);
  UEDATX = (line_coding.bParityType);
  UEDATX = (line_coding.bDataBits);
  UEINTX &= ~(1 << TXINI);
  
  while(! (UEINTX & (1 << TXINI)));
  
  while(!(UEINTX & (1 << RXOUTI)));
  UEINTX &= ~(1 << RXOUTI);
}

void cdc_set_line_coding (void)
{
  UEINTX &= ~(1 << RXSTPI);           //Шлем ACK
  while (!(UEINTX & (1 << RXOUTI)));  //Ждем данных
  
  LSB0(line_coding.dwDTERate) = UEDATX; //Читаем буффер
  LSB1(line_coding.dwDTERate) = UEDATX;
  LSB2(line_coding.dwDTERate) = UEDATX;
  LSB3(line_coding.dwDTERate) = UEDATX;
  line_coding.bCharFormat = UEDATX;
  line_coding.bParityType = UEDATX;
  line_coding.bDataBits = UEDATX;
  UEINTX &= ~(1 << RXOUTI);
  UEINTX &= ~(1 << TXINI);
}
        
BOOL usb_user_read_request(U16 type_request)
{
U8  descriptor_type ;
U8  string_type     ;
U8 req;
req = type_request & 0xFF;

        string_type     = UEDATX;
	descriptor_type = UEDATX;
	switch(req)
	{
	case GET_LINE_CODING:
		cdc_get_line_coding();
		return TRUE;

  	case SET_LINE_CODING:
		cdc_set_line_coding();
                return TRUE;

	case SET_CONTROL_LINE_STATE:
		cdc_set_control_line_state();
                return TRUE;

 	default:
		return FALSE;

	}
}

void START_PLL_and_USB_module(void)
{
  stop_PLL();
  init_pll();
  USB_reset();
  USB_enable();
}
