/* Application USB enum keeps the ST ABI without occupying the C bool name. */
#ifndef DOMOVOI_USB_BOOL_H
#define DOMOVOI_USB_BOOL_H
typedef enum { USB_FALSE = 0, USB_TRUE = 1 } usb_bool_t;
#ifndef FALSE
#define FALSE USB_FALSE
#define TRUE USB_TRUE
#endif
#endif
