#include <stdlib.h>
#include <math.h>
#include <gccore.h>
#include <unistd.h>
#include <fat.h>
#include <ogcsys.h>
#include <string.h>
#include <wiiuse/wpad.h>
#include <dirent.h>

#define WU_VENDOR_ID       0x057E //Nintendo
#define WU_PRODUCT_ID      0x0337 //USB GameCube Adapter
#define	WU_HEAP_SIZE       4096
#define WU_DEV_MAX         7

s32 wu_hId = -1;
s32 wu_dev_id_list[WU_DEV_MAX];
s32 wu_fd = -1;
u8 wu_bInEndpointAddress;
u8 wu_bOutEndpointAddress;
u16 wu_wInEndpointSize;
u16 wu_wOutEndpointSize;
u8 wu_isConnected;
uint8_t ATTRIBUTE_ALIGN(32) wu_previousPadData[37];
uint8_t ATTRIBUTE_ALIGN(32) wu_currentPadData[37];
u16 wu_padDataSize;


static lwp_t usbAdapterThread = LWP_THREAD_NULL;
void * usbAdapterThreadFunction();

s32 usbAdapterThreadCancelRequested = 0;
s32 usbAdapterPollInBackground = 0;

void USBAdapter_ReadBackground();
void USBAdapter_ReadBackgroundCallBack();

void USBAdapter_SetPollInBackground(s32 value)
{
	usbAdapterPollInBackground = value;
}

s32 USBAdapter_DisconnectCallback(s32 result, void *usrdata)
{
	wu_isConnected = 0;
	return 1;
}

s32 USBAdapter_Init()
{


	if (USB_Initialize() != IPC_OK)
		return -1;

	if (wu_hId > 0) //Already initialized
		return 0;
	memset(wu_previousPadData, 0, 37);
	memset(wu_currentPadData, 0, 37);
	wu_padDataSize = 37;
	wu_hId = iosCreateHeap(WU_HEAP_SIZE);
	int i;
	for (i = 0; i < WU_DEV_MAX; ++i)
		wu_dev_id_list[i] = 0;

	if (wu_hId < 0)
		return -1;

	return 0;
}



s32 USBAdapter_Open()
{
	
	if (wu_hId < 0)
	{
		return -1;
	}
	if (wu_isConnected) //Already connected
	{
		return 0;
	}
	u8 dev_count;
	usb_device_entry *dev_entry;
	dev_entry = (usb_device_entry *)iosAlloc(wu_hId, WU_DEV_MAX * sizeof(usb_device_entry));
	if (dev_entry == NULL)
	{
		wu_isConnected = 0;
		return -2;
	}
	memset(dev_entry, 0x0, WU_DEV_MAX * sizeof(usb_device_entry));

	//USB device list
	if (USB_GetDeviceList(dev_entry, WU_DEV_MAX, USB_CLASS_HID, &dev_count) < 0)
	{
		wu_isConnected = 0;
		iosFree(wu_hId, dev_entry);
		return -3;
	}
	int i, found = 0;
	for (i = 0; i < dev_count; i++)
	{
		if (dev_entry[i].vid == WU_VENDOR_ID && dev_entry[i].pid == WU_PRODUCT_ID)
		{
			found = 1;
			break;
		}
	}
	if (!found)
	{
		iosFree(wu_hId, dev_entry);
		wu_isConnected = 0;
		return -4;
	}

	wu_fd = -1;
	if (USB_OpenDevice(dev_entry[i].device_id, WU_VENDOR_ID, WU_PRODUCT_ID, &wu_fd) < 0)
	{
		iosFree(wu_hId, dev_entry);
		wu_isConnected = 0;
		return -5;
	}
	//wu_fd = IOS_Open("/dev/usb/hid/0x57e/0x337",0);
	if (wu_fd < 0)
		return -5;
	if (USB_DeviceRemovalNotifyAsync(wu_fd, &USBAdapter_DisconnectCallback, 0) < 0)
	{
		USB_CloseDevice(&wu_fd);
		wu_fd = -1;
		//iosFree(wu_hId, dev_entry);
		wu_isConnected = 0;
		return -6;
	}

	/*usb_devdesc *dev_desc;
	dev_desc = (usb_devdesc *)iosAlloc(wu_hId, sizeof(usb_devdesc));
	if (USB_GetDescriptors(wu_fd, dev_desc) < 0)
	{
	USB_CloseDevice(&wu_fd);
	wu_fd = -1;
	iosFree(wu_hId, dev_entry);
	wu_isConnected = 0;
	return -7;
	}


	u8 bEndpointAddress = dev_desc->configurations[0].interfaces[0].endpoints[0].bEndpointAddress;

	if (bEndpointAddress >= 0x80)
	{
	wu_bInEndpointAddress = bEndpointAddress;
	wu_wInEndpointSize = dev_desc->configurations[0].interfaces[0].endpoints[0].wMaxPacketSize;
	wu_bOutEndpointAddress = dev_desc->configurations[0].interfaces[0].endpoints[1].bEndpointAddress;
	wu_wOutEndpointSize = dev_desc->configurations[0].interfaces[0].endpoints[1].wMaxPacketSize;
	}
	else
	{
	wu_bOutEndpointAddress = bEndpointAddress;
	wu_wOutEndpointSize = dev_desc->configurations[0].interfaces[0].endpoints[0].wMaxPacketSize;
	wu_bInEndpointAddress = dev_desc->configurations[0].interfaces[0].endpoints[1].bEndpointAddress;
	wu_wInEndpointSize = dev_desc->configurations[0].interfaces[0].endpoints[1].wMaxPacketSize;
	}
	USB_FreeDescriptors(dev_desc);
	*/

	wu_bInEndpointAddress = 0x81;
	wu_wInEndpointSize = 37;
	wu_bOutEndpointAddress = 2;
	wu_wOutEndpointSize = 5;




	//usb_devdesc devdesc = dev_desc->configurations;

	uint8_t ATTRIBUTE_ALIGN(32) buf[1];
	buf[0] = 0x13;
	if (USB_WriteIntrMsg(wu_fd, wu_bOutEndpointAddress, 1, buf) < 0)
	{
		USB_CloseDevice(&wu_fd);
		wu_fd = -1;
		iosFree(wu_hId, dev_entry);
		wu_isConnected = 0;
		return -8;
	}

	wu_isConnected = 1;
	iosFree(wu_hId, dev_entry);
	return 0;
}



s32 USBAdapter_Read()
{
	wu_padDataSize = wu_wInEndpointSize;
	uint8_t ATTRIBUTE_ALIGN(32) inData[wu_wInEndpointSize];
	memset(inData, 0, wu_wInEndpointSize);
	s32 ret = USB_ReadIntrMsg(wu_fd, wu_bInEndpointAddress, wu_wInEndpointSize, inData);
	if (ret < 0)
		return ret;
	memcpy(wu_previousPadData, wu_currentPadData, wu_wInEndpointSize);
	memcpy(wu_currentPadData, inData, wu_wInEndpointSize);
	wu_padDataSize = wu_wInEndpointSize;
	return ret;

}

void USBAdapter_ReadBackground()
{
	USB_ReadIntrMsgAsync(wu_fd, 0x02, 37, (void*)0x901C4632, &USBAdapter_ReadBackgroundCallBack, NULL);
}
void USBAdapter_ReadBackgroundCallBack()
{
	USBAdapter_ReadBackground();
}


void USBAdapter_Start()
{
	LWP_CreateThread(&usbAdapterThread, usbAdapterThreadFunction, NULL, NULL, 0, 16);
	LWP_ResumeThread(usbAdapterThread);
}

void * usbAdapterThreadFunction()
{
	while (usbAdapterThreadCancelRequested == 0)
	{
		s32 ret = USBAdapter_Init();
		if (ret < 0)
		{
			usleep(1000);
			continue;
		}
		usleep(1000);

		ret = USBAdapter_Open();
		if (ret < 0)
		{
			usleep(1000);
			continue;
		}

		usleep(1000);
		bool error = false;
		while (usbAdapterThreadCancelRequested == 0 && !error)
		{
			if (usbAdapterPollInBackground)
			{
				//USBAdapter_ReadBackground();
				return;
			}
			if (!wu_isConnected)
				error = true;
			usleep(1000);
		}
	}
	USB_CloseDevice(&wu_fd);
}

void USBAdapter_Stop()
{
	usbAdapterThreadCancelRequested = 1;
	usleep(1000);
}

u16 UPAD_ButtonsUp(int pad)
{
	u16 ret = 0;
	int index = pad * 9 + 1;
	if (wu_currentPadData[index] == 0)
		return 0;
	u8 currentData1 = wu_currentPadData[index + 1];
	u8 previousData1 = wu_previousPadData[index + 1];
	u8 currentData2 = wu_currentPadData[index + 2];
	u8 previousData2 = wu_previousPadData[index + 2];

	if (((currentData1 & (1 << 0)) == 0) &&
		((previousData1 & (1 << 0)) != 0))
		ret |= PAD_BUTTON_A;
	if (((currentData1 & (1 << 1)) == 0) &&
		((previousData1 & (1 << 1)) != 0))
		ret |= PAD_BUTTON_B;
	if (((currentData1 & (1 << 2)) == 0) &&
		((previousData1 & (1 << 2)) != 0))
		ret |= PAD_BUTTON_X;
	if (((currentData1 & (1 << 3)) == 0) &&
		((previousData1 & (1 << 3)) != 0))
		ret |= PAD_BUTTON_Y;
	if (((currentData1 & (1 << 4)) == 0) &&
		((previousData1 & (1 << 4)) != 0))
		ret |= PAD_BUTTON_LEFT;
	if (((currentData1 & (1 << 5)) == 0) &&
		((previousData1 & (1 << 5)) != 0))
		ret |= PAD_BUTTON_RIGHT;
	if (((currentData1 & (1 << 6)) == 0) &&
		((previousData1 & (1 << 6)) != 0))
		ret |= PAD_BUTTON_DOWN;
	if (((currentData1 & (1 << 7)) == 0) &&
		((previousData1 & (1 << 7)) != 0))
		ret |= PAD_BUTTON_UP;
	if (((currentData2 & (1 << 0)) == 0) &&
		((previousData2 & (1 << 0)) != 0))
		ret |= PAD_BUTTON_START;
	if (((currentData2 & (1 << 1)) == 0) &&
		((previousData2 & (1 << 1)) != 0))
		ret |= PAD_TRIGGER_Z;
	if (((currentData2 & (1 << 2)) == 0) &&
		((previousData2 & (1 << 2)) != 0))
		ret |= PAD_TRIGGER_R;
	if (((currentData2 & (1 << 3)) == 0) &&
		((previousData2 & (1 << 3)) != 0))
		ret |= PAD_TRIGGER_L;
	return ret;

}

u16 UPAD_ButtonsDown(int pad)
{
	u16 ret = 0;
	int index = pad * 9 + 1;
	if (wu_currentPadData[index] == 0)
		return 0;
	u8 currentData1 = wu_currentPadData[index + 1];
	u8 previousData1 = wu_previousPadData[index + 1];
	u8 currentData2 = wu_currentPadData[index + 2];
	u8 previousData2 = wu_previousPadData[index + 2];

	if (((currentData1 & (1 << 0)) != 0) &&
		((previousData1 & (1 << 0)) == 0))
		ret |= PAD_BUTTON_A;
	if (((currentData1 & (1 << 1)) != 0) &&
		((previousData1 & (1 << 1)) == 0))
		ret |= PAD_BUTTON_B;
	if (((currentData1 & (1 << 2)) != 0) &&
		((previousData1 & (1 << 2)) == 0))
		ret |= PAD_BUTTON_X;
	if (((currentData1 & (1 << 3)) != 0) &&
		((previousData1 & (1 << 3)) == 0))
		ret |= PAD_BUTTON_Y;
	if (((currentData1 & (1 << 4)) != 0) &&
		((previousData1 & (1 << 4)) == 0))
		ret |= PAD_BUTTON_LEFT;
	if (((currentData1 & (1 << 5)) != 0) &&
		((previousData1 & (1 << 5)) == 0))
		ret |= PAD_BUTTON_RIGHT;
	if (((currentData1 & (1 << 6)) != 0) &&
		((previousData1 & (1 << 6)) == 0))
		ret |= PAD_BUTTON_DOWN;
	if (((currentData1 & (1 << 7)) != 0) &&
		((previousData1 & (1 << 7)) == 0))
		ret |= PAD_BUTTON_UP;
	if (((currentData2 & (1 << 0)) != 0) &&
		((previousData2 & (1 << 0)) == 0))
		ret |= PAD_BUTTON_START;
	if (((currentData2 & (1 << 1)) != 0) &&
		((previousData2 & (1 << 1)) == 0))
		ret |= PAD_TRIGGER_Z;
	if (((currentData2 & (1 << 2)) != 0) &&
		((previousData2 & (1 << 2)) == 0))
		ret |= PAD_TRIGGER_R;
	if (((currentData2 & (1 << 3)) != 0) &&
		((previousData2 & (1 << 3)) == 0))
		ret |= PAD_TRIGGER_L;
	return ret;
}
u16 UPAD_ButtonsHeld(int pad)
{
	u16 ret = 0;
	int index = pad * 9 + 1;
	if (wu_currentPadData[index] == 0)
		return 0;
	u8 currentData1 = wu_currentPadData[index + 1];
	u8 previousData1 = wu_previousPadData[index + 1];
	u8 currentData2 = wu_currentPadData[index + 2];
	u8 previousData2 = wu_previousPadData[index + 2];

	if (((currentData1 & (1 << 0)) != 0) &&
		((previousData1 & (1 << 0)) != 0))
		ret |= PAD_BUTTON_A;
	if (((currentData1 & (1 << 1)) != 0) &&
		((previousData1 & (1 << 1)) != 0))
		ret |= PAD_BUTTON_B;
	if (((currentData1 & (1 << 2)) != 0) &&
		((previousData1 & (1 << 2)) != 0))
		ret |= PAD_BUTTON_X;
	if (((currentData1 & (1 << 3)) != 0) &&
		((previousData1 & (1 << 3)) != 0))
		ret |= PAD_BUTTON_Y;
	if (((currentData1 & (1 << 4)) != 0) &&
		((previousData1 & (1 << 4)) != 0))
		ret |= PAD_BUTTON_LEFT;
	if (((currentData1 & (1 << 5)) != 0) &&
		((previousData1 & (1 << 5)) != 0))
		ret |= PAD_BUTTON_RIGHT;
	if (((currentData1 & (1 << 6)) != 0) &&
		((previousData1 & (1 << 6)) != 0))
		ret |= PAD_BUTTON_DOWN;
	if (((currentData1 & (1 << 7)) != 0) &&
		((previousData1 & (1 << 7)) != 0))
		ret |= PAD_BUTTON_UP;
	if (((currentData2 & (1 << 0)) != 0) &&
		((previousData2 & (1 << 0)) != 0))
		ret |= PAD_BUTTON_START;
	if (((currentData2 & (1 << 1)) != 0) &&
		((previousData2 & (1 << 1)) != 0))
		ret |= PAD_TRIGGER_Z;
	if (((currentData2 & (1 << 2)) != 0) &&
		((previousData2 & (1 << 2)) != 0))
		ret |= PAD_TRIGGER_R;
	if (((currentData2 & (1 << 3)) != 0) &&
		((previousData2 & (1 << 3)) != 0))
		ret |= PAD_TRIGGER_L;
	return ret;
}

s8 UPAD_SubStickX(int pad){
	int index = pad * 9 + 1;
	if (wu_currentPadData[index] == 0)
		return 0;
	return wu_currentPadData[index + 5] - 127;

}
s8 UPAD_SubStickY(int pad)
{
	int index = pad * 9 + 1;
	if (wu_currentPadData[index] == 0)
		return 0;
	return wu_currentPadData[index + 6] - 127;
}

s8 UPAD_StickX(int pad)
{
	int index = pad * 9 + 1;
	if (wu_currentPadData[index] == 0)
		return 0;
	return wu_currentPadData[index + 3] - 127;
}
s8 UPAD_StickY(int pad)
{
	int index = pad * 9 + 1;
	if (wu_currentPadData[index] == 0)
		return 0;
	return wu_currentPadData[index + 4] - 127;
}

u8 UPAD_TriggerL(int pad)
{
	int index = pad * 9 + 1;
	if (wu_currentPadData[index] == 0)
		return 0;
	return wu_currentPadData[index + 7] - 127;
}
u8 UPAD_TriggerR(int pad)
{
	int index = pad * 9 + 1;
	if (wu_currentPadData[index] == 0)
		return 0;
	return wu_currentPadData[index + 8] - 127;
}

s32 UPAD_ScanPads()
{
	return USBAdapter_Read();
}