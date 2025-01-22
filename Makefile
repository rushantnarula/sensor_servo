PROJECT_NAME := sensor_servo
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD

-include ./proj_config.mk

ifeq ($(origin BL60X_SDK_PATH), undefined)
$(error ****** Please SET BL60X_SDK_PATH ******)
endif

COMPONENTS_NETWORK := sntp dns_server
COMPONENTS_BLSYS   := bltime blfdt blmtd blota bloop loopadc looprt loopset
COMPONENTS_VFS     := romfs
COMPONENTS_UTILS   := cjson

INCLUDE_COMPONENTS += freertos_riscv_ram bl602 bl602_std bl602_wifi bl602_wifidrv hal_drv lwip lwip_dhcpd vfs yloop utils cli httpc netutils blog blog_testc
INCLUDE_COMPONENTS += sensor_servo
INCLUDE_COMPONENTS += sensor_servo_new
INCLUDE_COMPONENTS += easyflash4
INCLUDE_COMPONENTS += cgi
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(COMPONENTS_UTILS)
INCLUDE_COMPONENTS += $(PROJECT_NAME)

include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk
