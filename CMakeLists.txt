# Some parts copied from https://github.com/mkleemann/cmake-avr with the following LICENSE
# "THE ANY BEVERAGE-WARE LICENSE" (Revision 42 - based on beer-ware
# license):
# <dev@layer128.net> wrote this file. As long as you retain this notice
# you can do whatever you want with this stuff. If we meet some day, and
# you think this stuff is worth it, you can buy me a be(ve)er(age) in
# return. (I don't like beer much.)
#
# Matthias Kleemann

cmake_minimum_required(VERSION 3.0)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_C_FLAGS_RELEASE "-Os")
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(agsklingel C ASM)

find_program(AVR_CC avr-gcc REQUIRED)
find_program(AVR_OBJCOPY avr-objcopy REQUIRED)
find_program(AVR_SIZE_TOOL avr-size REQUIRED)
find_program(AVR_OBJDUMP avr-objdump REQUIRED)
find_program(AVRDUDE avrdude REQUIRED)
find_path(AVR_INCLUDE_DIR NAMES avr/io.h util/delay.h HINTS "${AVR_CC}/../../avr/include" "/usr/lib/avr/include" REQUIRED)

set(CMAKE_C_COMPILER ${AVR_CC})
set(CMAKE_CXX_COMPILER ${AVR_CXX})
set(CMAKE_ASM_COMPILER ${AVR_CC})

set(UPDI_PROGRAMMER_PORT "/dev/ttyUSB0" CACHE STRING "The Port on the computer where the UPDI programmer is connected (usually COMXX on Windows and /dev/ttyUSBX on Linux)")
set(RS485_PROGRAMMER_PORT "/dev/ttyUSB1" CACHE STRING "The Port on the computer where the RS485 programmer is connected (usually COMXX on Windows and /dev/ttyUSBX on Linux)")
set(MCU_SPEED 3333333 CACHE STRING "The Clock speed of the target MCU")
set(PROGRAMMING_SPEED 115200 CACHE STRING "The Baudrate when programming via UPDI")

##################################################################################
# compiler options for all build types
##################################################################################
include_directories("${AVR_INCLUDE_DIR}")
add_definitions("-DF_CPU=${MCU_SPEED}UL")
add_definitions("-D__AVR_ATtiny1616__")
add_definitions("-fpack-struct")
add_definitions("-fshort-enums")
add_definitions("-Wall")
# http://gcc.gnu.org/onlinedocs/gcc-4.8.2/gcc/Alternate-Keywords.html#Alternate-Keywords
# [...]-pedantic and other options cause warnings for many GNU C extensions. You can prevent such warnings within
# one expression by writing __extension__ before the expression. __extension__ has no effect aside from this.[...] 
add_definitions("-pedantic")
add_definitions("-funsigned-char")
add_definitions("-funsigned-bitfields")
add_definitions("-ffunction-sections")
add_definitions("-c")
add_definitions("-std=gnu99")


set(EXE_NAME "${CMAKE_PROJECT_NAME}")
set(SRC_FILES main.c disp.s)
add_executable("${EXE_NAME}.elf" EXCLUDE_FROM_ALL ${SRC_FILES})
set_target_properties("${EXE_NAME}.elf" PROPERTIES
	COMPILE_FLAGS "-mmcu=attiny1616"
	LINK_FLAGS "-mmcu=attiny1616 -Wl,--gc-sections -Wl,--section-start=.text=0x400 -mrelax -Wl,-Map,'${CMAKE_BINARY_DIR}/${EXE_NAME}.map'")
add_custom_command(
  OUTPUT "${EXE_NAME}.hex"
  COMMAND "${AVR_OBJCOPY}" -j .text -j .data -O ihex "${EXE_NAME}.elf" "${EXE_NAME}.hex"
  COMMAND "${AVR_SIZE_TOOL}" "${EXE_NAME}.elf"
  DEPENDS "${EXE_NAME}.elf"
)

add_custom_target("${EXE_NAME}" ALL DEPENDS "${EXE_NAME}.hex")
set_target_properties("${EXE_NAME}" PROPERTIES OUTPUT_NAME "${EXE_NAME}.elf")

add_custom_target(program
	${AVRDUDE} -p t1616 -c arduino -D -b "${PROGRAMMING_SPEED}" -P "${RS485_PROGRAMMER_PORT}" -U "flash:w:${EXE_NAME}.hex"
	DEPENDS "${EXE_NAME}.hex"
)



set(EXE_NAME "bootloader")
set(SRC_FILES bootloader.s bootloader.c)
add_executable("${EXE_NAME}.elf" EXCLUDE_FROM_ALL ${SRC_FILES})
set_target_properties("${EXE_NAME}.elf" PROPERTIES
	COMPILE_FLAGS "-mmcu=attiny1616"
	LINK_FLAGS "-mmcu=attiny1616 -Wl,--gc-sections -mrelax -Wl,-Map,'${CMAKE_BINARY_DIR}/${EXE_NAME}.map'")
add_custom_command(
  OUTPUT "${EXE_NAME}.hex"
  COMMAND "${AVR_OBJCOPY}" -j .text -j .data -O ihex "${EXE_NAME}.elf" "${EXE_NAME}.hex"
  COMMAND "${AVR_SIZE_TOOL}" "${EXE_NAME}.elf"
  DEPENDS "${EXE_NAME}.elf"
)

add_custom_target("${EXE_NAME}" ALL DEPENDS "${EXE_NAME}.hex")
set_target_properties("${EXE_NAME}" PROPERTIES OUTPUT_NAME "${EXE_NAME}.elf")

add_custom_target(programbl
	${AVRDUDE} -p t1616 -c serialupdi -b "${PROGRAMMING_SPEED}" -P "${UPDI_PROGRAMMER_PORT}" -U "flash:w:${EXE_NAME}.hex"
	DEPENDS "${EXE_NAME}.hex"
)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)
