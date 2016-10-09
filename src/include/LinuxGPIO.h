#ifndef __LinuxGPIO_h
#define __LinuxGPIO_h

#include <map>

#include "ParallelPort.h"

/** \file */


/**
 * An implementation of the IO interface which uses the Linux sysfs GPIO
 * driver.
 */
class LinuxGPIO : public ParallelPort
{
public:
    /**
     * This constructor will open and initialize the parallel port ppdev
     * device. Both DevFS and pre-DevFS setups are supported.
     *
     * \param port The number of the port to use. 
     * \throws runtime_error Contains a textual description of the error.
     */
    LinuxGPIO(int port);

    /** Destructor */
    ~LinuxGPIO();

    void set_pin_state (
        const char *name,
        short reg,
        short bit, 
        short invert,
        bool state
    );

    bool get_pin_state (
        const char *name,
        short reg,
        short bit,
        short invert
    );

private:
    std::map<int,int> gpiofd;

    int gpio_setup (
        short reg,
        short bit,
        short invert,
        short direction
    );

    int  gpio_open         (short bit);

    void gpio_close_all();

    void gpio_export       (short bit);
    void gpio_unexport     (short bit);
    void gpio_set_direction(short bit, short direction);

    int gpio_open_write_close(const char *name, const char *valstr);
};

#endif
