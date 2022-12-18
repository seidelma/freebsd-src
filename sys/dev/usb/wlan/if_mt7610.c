#include <sys/param.h>
#include <sys/eventhandler.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/linker.h>
#include <sys/firmware.h>
#include <sys/kdb.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include "usbdevs.h"

#include "if_mt7610.h"

#define USB_DEBUG_VAR   mt7610_debug
#include <dev/usb/usb_debug.h>
#include <dev/usb/usb_msctest.h>

enum {
	MT7610_DEBUG_PROBE	= 0x00000001,   /* device probe */
	MT7610_DEBUG_ATTACH     = 0x00000002,   /* device attach */
	MT7610_DEBUG_DETACH     = 0x00000004,   /* device detach */
	MT7610_DEBUG_ANY        = 0xffffffff
};

#define MT7610_DEBUG
int mt7610_debug = 0;

#ifdef  MT7610_DEBUG
#define MT7610_DPRINTF(_sc, _m, ...) do {		   \
	if (mt7610_debug & (_m))			   \
		device_printf((_sc)->sc_dev, __VA_ARGS__); \
} while(0)
#else
#define MT7610_DPRINTF(_sc, _m, ...)       do { (void) _sc; } while (0)
#endif

/* From sys/dev/usb/usbdevs */
static const STRUCT_USB_HOST_ID mt7610_devs[] = {
#define MT7610_DEV(v,p)    { USB_VP(USB_VENDOR_##v, USB_PRODUCT_##v##_##p) }
#define MT7610_DEV_EJECT(v,p)      \
	{ USB_VPI(USB_VENDOR_##v, USB_PRODUCT_##v##_##p, MT7610_EJECT) }
#define MT7610_EJECT       1
	MT7610_DEV(ASUS,		USBAC51),
#undef MT7610_DEV_EJECT
#undef MT7610_DEV
};

static eventhandler_tag mt7610_etag;

/* module function declarations */

static void	mt7610_autoinst(void *,
		    struct usb_device *, struct usb_attach_arg *);
static int	mt7610_driver_loaded(struct module *, int, void *);



/* Checks to see if we have a device to support when
   the module is kldload'ed, as long as the device is
   not actively being ejected (I think) */
static void
mt7610_autoinst(void *arg, struct usb_device *udev,
    struct usb_attach_arg *uaa)
{
	struct usb_interface *iface;
	struct usb_interface_descriptor *id;

	printf("In mt7610_autoinst\n");

	if (uaa->dev_state != UAA_DEV_READY)
		return;

	iface = usbd_get_iface(udev, 0);
	if (iface == NULL)
		return;
	id = iface->idesc;
	if (id == NULL || id->bInterfaceClass != UICLASS_MASS)
		return;
	if (usbd_lookup_id_by_uaa(mt7610_devs, sizeof(mt7610_devs), uaa))
		return;

	/* If we in fact ARE being ejected, inform interested parties
	   by setting the dev_state appropriately */
	if (usb_msc_eject(udev, 0, MSC_EJECT_STOPUNIT) == 0)
		uaa->dev_state = UAA_DEV_EJECTING;
}

/* Handles module load/unload to/from the running kernel */
static int
mt7610_driver_loaded(struct module *mod, int mode, void *arg)
{
	printf("In mt7610_driver_loaded\n");
	switch (mode) {
	case MOD_LOAD:
		mt7610_etag = EVENTHANDLER_REGISTER(usb_dev_configured,
		    mt7610_autoinst, NULL, EVENTHANDLER_PRI_ANY);
		break;
	case MOD_UNLOAD:
		EVENTHANDLER_DEREGISTER(usb_dev_configured, mt7610_etag);
		break;
	default:
		return (EOPNOTSUPP);
	}
	return (0);
}

/* Callback from device(9)'s DEVMETHOD(device_probe) macro */
static int
mt7610_match(device_t self)
{
	printf("In mt7610_match\n");
	struct usb_attach_arg *uaa = device_get_ivars(self);

	if (uaa->usb_mode != USB_MODE_HOST)
		return (ENXIO);
	if (uaa->info.bConfigIndex != 0)
		return (ENXIO);

	return (usbd_lookup_id_by_uaa(mt7610_devs, sizeof(mt7610_devs), uaa));
}

/* Callback from device(9)'s DEVMETHOD(device_attach) macro */
static int
mt7610_attach(device_t self)
{
	printf("In mt7610_attach\n");
	struct mt7610_softc *sc = device_get_softc(self);
	MT7610_DPRINTF(sc, MT7610_DEBUG_ATTACH, "Finished device_get_softc()\n");
	return 0;
}

/* Callback from device(9)'s DEVMETHOD(device_detach) macro */
static int
mt7610_detach(device_t self)
{
	printf("In mt7610_detach\n");
	struct mt7610_softc *sc = device_get_softc(self);
	MT7610_DPRINTF(sc, MT7610_DEBUG_DETACH, "Finished device_get_softc()\n");
	return 0;
}


/* Driver boilerplate */
static device_probe_t		mt7610_match;
static device_attach_t		mt7610_attach;
static device_detach_t		mt7610_detach;

static device_method_t mt7610_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe, 	mt7610_match),
	DEVMETHOD(device_attach,	mt7610_attach),
	DEVMETHOD(device_detach,	mt7610_detach),
	DEVMETHOD_END
};

static driver_t mt7610_driver = {
	.name = "mt7610",
	.methods = mt7610_methods,
	.size = sizeof(struct mt7610_softc)
};

DRIVER_MODULE(mt7610, uhub, mt7610_driver, mt7610_driver_loaded, NULL);
MODULE_DEPEND(mt7610, wlan, 1, 1, 1);
MODULE_DEPEND(mt7610, usb, 1, 1, 1);
/* not yet
MODULE_DEPEND(mt7610, firmware, 1, 1, 1);
*/
MODULE_VERSION(mt7610, 1);
