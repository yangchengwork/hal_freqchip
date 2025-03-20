#
# Copyright (c) 2025 Freqchip
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_named(hal_freqchip)

zephyr_include_directories(components/drivers/device/fr30xx)
zephyr_include_directories(components/drivers/peripheral/drv_common)
zephyr_include_directories(components/drivers/peripheral/Inc)
zephyr_include_directories(components/modules/common/include)
zephyr_include_directories(components/modules/crc)
zephyr_include_directories(examples/common/btdm)

zephyr_library_sources(components/drivers/device/fr30xx/system_fr30xx.c)
zephyr_library_sources(components/drivers/device/fr30xx/trim_fr30xx.c)
zephyr_library_sources(components/drivers/peripheral/Src/driver_efuse.c)
zephyr_library_sources(components/drivers/peripheral/Src/driver_flash.c)
zephyr_library_sources(components/drivers/peripheral/Src/driver_frspim.c)
zephyr_library_sources(components/drivers/peripheral/Src/driver_gpio.c)
zephyr_library_sources(components/drivers/peripheral/Src/driver_pmu.c)
zephyr_library_sources(components/drivers/peripheral/Src/driver_qspi.c)
zephyr_library_sources(components/drivers/peripheral/Src/driver_uart.c)
zephyr_library_sources(components/modules/common/src/co_util.c)
zephyr_library_sources(components/modules/crc/crc32.c)
zephyr_library_sources(examples/common/btdm/controller.c)
zephyr_library_sources(examples/common/btdm/controller_code_revb.c)
zephyr_library_sources(examples/common/btdm/controller_code_revc.c)
zephyr_library_sources(examples/common/btdm/SWD.c)
