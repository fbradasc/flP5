
INCLUDE_DIRECTORIES (
    ${FLP5_SOURCE_DIR}/src/include
    ${FLP5_SOURCE_DIR}/src/gui
    ${FLP5_SOURCE_DIR}/src/lib
    ${FLP5_SOURCE_DIR}/src/lib/devices
    ${FLP5_SOURCE_DIR}/src/lib/devices/Microchip
    ${FLP5_SOURCE_DIR}/src/lib/devices/Microchip/PIC
)

SET ( FLP5_SOURCES
#
# Micellaneus & utility sources
#
    lib/RegularExpression.cxx
    lib/Util.cxx
    lib/DataBuffer.cxx
    lib/Preferences.cxx
#
# Hex file I/O
#
    lib/HexFile.cxx
    lib/HexFile_ihx16.cxx
    lib/HexFile_ihx8.cxx
#
# Parallel port I/O
#
    lib/IO.cxx
    lib/LptPorts.cxx
    lib/ParallelPort.cxx
    lib/DlPortDriver.cxx
    lib/DirectPPIO.cxx
    lib/LinuxPPDevIO.cxx
    lib/LinuxGPIO.cxx
#
# Device definition & programming algorithms
#
    lib/Device.cxx
    lib/devices/Microchip/Microchip.cxx
    lib/devices/Microchip/PIC/Pic.cxx
    lib/devices/Microchip/PIC/Pic16.cxx
    lib/devices/Microchip/PIC/Pic12f6xx.cxx
    lib/devices/Microchip/PIC/Pic16f6xx.cxx
    lib/devices/Microchip/PIC/Pic16f7x.cxx
    lib/devices/Microchip/PIC/Pic16f8xx.cxx
    lib/devices/Microchip/PIC/Pic16f88x.cxx
    lib/devices/Microchip/PIC/Pic16f87xA.cxx
    lib/devices/Microchip/PIC/Pic18.cxx
    lib/devices/Microchip/PIC/Pic18fxx20.cxx
    lib/devices/Microchip/PIC/Pic18f2xx0.cxx
#
# Useful widgets
#
    gui/Fl_RaiseButton.cxx
    gui/Fl_Sorted_Choice.cxx
    gui/Fl_Tree_Browser.cxx
#
# User interface
#
    gui/fl_call_main.c
    gui/main_flP5.cxx
    gui/flP5.cxx
)

IF ( WIN32 )
    ADD_EXECUTABLE ( flP5 WIN32 ${FLP5_SOURCES} )
ELSE ( WIN32 )
    ADD_EXECUTABLE ( flP5 ${FLP5_SOURCES} )
ENDIF ( WIN32 )

INSTALL_TARGETS( ${INSTALL_PREFIX}/bin flP5 )
