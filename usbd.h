#include "USB_cdc_user_functions.h"

#define REQUEST_DEVICE_STATUS                 0x80
#define GET_DESCRIPTOR                        0x06

// Descriptor Types
#define DEVICE_DESCRIPTOR                     0x01
#define CONFIGURATION_DESCRIPTOR              0x02
#define STRING_DESCRIPTOR                     0x03
#define INTERFACE_DESCRIPTOR                  0x04
#define ENDPOINT_DESCRIPTOR                   0x05
#define DEVICE_QUALIFIER_DESCRIPTOR           0x06
#define OTHER_SPEED_CONFIGURATION_DESCRIPTOR  0x07

// Interface 0 descriptor
#define INTERFACE0_NB        0
#define ALTERNATE0           0
#define NB_ENDPOINT0         1
#define INTERFACE0_CLASS     0x02    // CDC ACM Com
#define INTERFACE0_SUB_CLASS 0x02
#define INTERFACE0_PROTOCOL  0x01
#define INTERFACE0_INDEX     0

// CDC CONFIGURATION
#define NB_INTERFACE       2
#define CONF_NB            1
#define CONF_INDEX         0
#define CONF_ATTRIBUTES    USB_CONFIG_BUSPOWERED
#define MAX_POWER          50      

#define TYPE_CONTROL             0
#define TYPE_ISOCHRONOUS         1
#define TYPE_BULK                2
#define TYPE_INTERRUPT           3

#define DIRECTION_OUT            0
#define DIRECTION_IN             1

#define SIZE_8                   0
#define SIZE_16                  1
#define SIZE_32                  2
#define SIZE_64                  3
#define SIZE_128                 4
#define SIZE_256                 5
#define SIZE_512                 6
#define SIZE_1024                7

#define ONE_BANK                 0
#define TWO_BANKS                1

#define NYET_ENABLED             0
#define NYET_DISABLED            1

#define USB_CONFIG_ATTRIBUTES_RESERVED    0x80
#define USB_CONFIG_BUSPOWERED            (USB_CONFIG_ATTRIBUTES_RESERVED | 0x00)
#define USB_CONFIG_SELFPOWERED           (USB_CONFIG_ATTRIBUTES_RESERVED | 0x40)
#define USB_CONFIG_REMOTEWAKEUP          (USB_CONFIG_ATTRIBUTES_RESERVED | 0x20)

#define USB_MN_LENGTH         5

#define USB_PN_LENGTH         16

#define USB_SN_LENGTH         0x05

#define LANGUAGE_ID           0x0409

// Interface 1 descriptor
#define INTERFACE1_NB        1
#define ALTERNATE1           0
#define NB_ENDPOINT1         2
#define INTERFACE1_CLASS     0x0A    // CDC ACM Data
#define INTERFACE1_SUB_CLASS 0
#define INTERFACE1_PROTOCOL  0
#define INTERFACE1_INDEX     0

// USB Endpoint 1 descriptor
// Bulk IN
#define ENDPOINT_NB_1       0x80 | TX_EP
#define EP_ATTRIBUTES_1     0x02          // BULK = 0x02, INTERUPT = 0x03
#define EP_SIZE_1           0x20
#define EP_INTERVAL_1       0x00

// USB Endpoint 2 descriptor
//Bulk OUT  RX endpoint
#define ENDPOINT_NB_2       RX_EP
#define EP_ATTRIBUTES_2     0x02          // BULK = 0x02, INTERUPT = 0x03
#define EP_SIZE_2           0x20
#define EP_INTERVAL_2       0x00

// USB Endpoint 3 descriptor
// Interrupt IN
#define TX_EP_SIZE          0x20
#define ENDPOINT_NB_3       0x80 | INT_EP
#define EP_ATTRIBUTES_3     0x03          // BULK = 0x02, INTERUPT = 0x03
#define EP_SIZE_3           TX_EP_SIZE
#define EP_INTERVAL_3       0xFF          //ms interrupt pooling from host

#define USB_SPECIFICATION     0x0200
#define DEVICE_CLASS          0x02      // CDC class
#define DEVICE_SUB_CLASS      0      // each configuration has its own sub-class
#define DEVICE_PROTOCOL       0      // each configuration has its own protocol
#define EP_CONTROL_LENGTH     64
#define VENDOR_ID             0x03EB // Atmel vendor ID = 03EBh
#define PRODUCT_ID            0x201F // ID устройства
#define RELEASE_NUMBER        0x1000
#define MAN_INDEX             0x00
#define PROD_INDEX            0x00
#define SN_INDEX              0x00
#define NB_CONFIGURATION      1

#define NB_ENDPOINTS          4  //!  number of endpoints in the application including control endpoint RX, TX, INT + control
#define TX_EP		      0x01
#define RX_EP		      0x02
#define INT_EP                0x03

typedef unsigned char   U8  ;
typedef unsigned short  U16 ;
typedef unsigned long  U32 ;

typedef struct
{
	U32 dwDTERate;
	U8 bCharFormat;
	U8 bParityType;
	U8 bDataBits;
}S_line_coding;

#define MSB(u16)  (((U8* )&u16)[1])
#define LSB(u16)  (((U8* )&u16)[0])

//! Usb Device Descriptor
typedef struct {
   U8      bLength;              //!< Size of this descriptor in bytes
   U8      bDescriptorType;      //!< DEVICE descriptor type
   U16     bscUSB;               //!< Binay Coded Decimal Spec. release
   U8      bDeviceClass;         //!< Class code assigned by the USB
   U8      bDeviceSubClass;      //!< Sub-class code assigned by the USB
   U8      bDeviceProtocol;      //!< Protocol code assigned by the USB
   U8      bMaxPacketSize0;      //!< Max packet size for EP0
   U16     idVendor;             //!< Vendor ID. ATMEL = 0x03EB
   U16     idProduct;            //!< Product ID assigned by the manufacturer
   U16     bcdDevice;            //!< Device release number
   U8      iManufacturer;        //!< Index of manu. string descriptor
   U8      iProduct;             //!< Index of prod. string descriptor
   U8      iSerialNumber;        //!< Index of S.N.  string descriptor
   U8      bNumConfigurations;   //!< Number of possible configurations
}  S_usb_device_descriptor;

//! Usb Configuration Descriptor
typedef struct {
   U8      bLength;              //!< size of this descriptor in bytes
   U8      bDescriptorType;      //!< CONFIGURATION descriptor type
   U16     wTotalLength;         //!< total length of data returned
   U8      bNumInterfaces;       //!< number of interfaces for this conf.
   U8      bConfigurationValue;  //!< value for SetConfiguration resquest
   U8      iConfiguration;       //!< index of string descriptor
   U8      bmAttibutes;          //!< Configuration characteristics
   U8      MaxPower;             //!< maximum power consumption
}  S_usb_configuration_descriptor;

//! Usb Interface Descriptor
typedef struct {
   U8      bLength;               //!< size of this descriptor in bytes
   U8      bDescriptorType;       //!< INTERFACE descriptor type
   U8      bInterfaceNumber;      //!< Number of interface
   U8      bAlternateSetting;     //!< value to select alternate setting
   U8      bNumEndpoints;         //!< Number of EP except EP 0
   U8      bInterfaceClass;       //!< Class code assigned by the USB
   U8      bInterfaceSubClass;    //!< Sub-class code assigned by the USB
   U8      bInterfaceProtocol;    //!< Protocol code assigned by the USB
   U8      iInterface;            //!< Index of string descriptor
}  S_usb_interface_descriptor;

//! Usb Endpoint Descriptor
typedef struct {
   U8      bLength;               //!< Size of this descriptor in bytes
   U8      bDescriptorType;       //!< ENDPOINT descriptor type
   U8      bEndpointAddress;      //!< Address of the endpoint
   U8      bmAttributes;          //!< Endpoint's attributes
   U16     wMaxPacketSize;        //!< Maximum packet size for this EP
   U8      bInterval;             //!< Interval for polling EP in ms
} S_usb_endpoint_descriptor;

//! Usb Device Qualifier Descriptor
typedef struct {
   U8      bLength;               //!< Size of this descriptor in bytes
   U8      bDescriptorType;       //!< Device Qualifier descriptor type
   U16     bscUSB;                //!< Binay Coded Decimal Spec. release
   U8      bDeviceClass;          //!< Class code assigned by the USB
   U8      bDeviceSubClass;       //!< Sub-class code assigned by the USB
   U8      bDeviceProtocol;       //!< Protocol code assigned by the USB
   U8      bMaxPacketSize0;       //!< Max packet size for EP0
   U8      bNumConfigurations;    //!< Number of possible configurations
   U8      bReserved;             //!< Reserved for future use, must be zero
}  S_usb_device_qualifier_descriptor;

//! Usb Language Descriptor
typedef struct {
   U8      bLength;               //!< size of this descriptor in bytes
   U8      bDescriptorType;       //!< STRING descriptor type
   U16     wlangid;               //!< language id
}  S_usb_language_id;

//struct usb_st_manufacturer
typedef struct {
   U8  bLength;               // size of this descriptor in bytes
   U8  bDescriptorType;       // STRING descriptor type
   U16 wstring[USB_MN_LENGTH];// unicode characters
} S_usb_manufacturer_string_descriptor;


//struct usb_st_product
typedef struct {
   U8  bLength;               // size of this descriptor in bytes
   U8  bDescriptorType;       // STRING descriptor type
   U16 wstring[USB_PN_LENGTH];// unicode characters
} S_usb_product_string_descriptor;


//struct usb_st_serial_number
typedef struct {
   U8  bLength;               // size of this descriptor in bytes
   U8  bDescriptorType;       // STRING descriptor type
   U16 wstring[USB_SN_LENGTH];// unicode characters
} S_usb_serial_number;

typedef struct
{
   S_usb_configuration_descriptor cfg;
   S_usb_interface_descriptor     ifc0;
	U8 CS_INTERFACE[19];
   S_usb_endpoint_descriptor      ep3;
   S_usb_interface_descriptor     ifc1;
   S_usb_endpoint_descriptor      ep1;
   S_usb_endpoint_descriptor      ep2;
} S_usb_user_configuration_descriptor;

void USB_reset(void);
void USB_enable(void);

void USB_get_descriptor(void);
void USB_Set_Address(void);

void USB_send_buffer(unsigned int len, unsigned char *ByteToSend);
void send_msg_to_console(unsigned int len, unsigned char __flash *flash_buffer);

BOOL usb_user_read_request(U16 type_request);
