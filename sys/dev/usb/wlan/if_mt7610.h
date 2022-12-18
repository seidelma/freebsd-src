#ifndef _IF_MT7610_H_
#define _IF_MT7610_H_

struct mt7610_softc;

struct mt7610_softc {
        device_t                        sc_dev;
        struct usb_device               *sc_udev;
};

#endif /* _IF_MT7610_H_ */
