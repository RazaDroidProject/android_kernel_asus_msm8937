#hebiao@wind-mobi.com 20171001 begin
# Linux driver folder
EXTRA_CFLAGS +=-Wno-error=date-time
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/gt9xx_android/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/misc/wind_device_info/

obj-y	+=  gt9xx.o
obj-y	+=  gt9xx_update.o
obj-y	+=  goodix_tool.o

#hebiao@wind-mobi.com 20171001 end
