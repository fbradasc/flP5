#if defined(linux) && defined(ENABLE_LINUX_GPIO)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

using namespace std;

#include "Util.h"
#include "LinuxGPIO.h"

#define GPIO_SETUP(name) \
{ \
    int fd = gpio_setup (\
        GET_PIN_REG(name), \
        GET_PIN_BIT(name), \
        GET_PIN_NEG(name), \
        GET_PIN_DIR(name)  \
    ); \
    \
    if (fd > 0) { \
        gpiofd[GET_PIN_BIT(name)] = fd; \
    } \
}

LinuxGPIO::LinuxGPIO(int port) : ParallelPort(port)
{
    try {
        GPIO_SETUP("icspClock"  );
        GPIO_SETUP("icspDataIn" );
        GPIO_SETUP("icspDataOut");
        GPIO_SETUP("icspVddOn"  );
        GPIO_SETUP("icspVppOn"  );
        GPIO_SETUP("selMinVdd"  );
        GPIO_SETUP("selProgVdd" );
        GPIO_SETUP("selMaxVdd"  );
        GPIO_SETUP("selVihhVpp" );

        this->off();
        vdd(VDD_TO_PRG);
    } catch (const char *error) {
        try {
            gpio_close_all();
        } catch (const char *close_error) {
            throw runtime_error (
                (const char *)Preferences::Name (
		            "%s\n%s",
                    close_error,
                    error
                )
            );
        }
        throw runtime_error(error);
    }
    fprintf (
        stderr,
        "LinuxGPIO::LinuxGPIO instantiated\n"
    );
}

LinuxGPIO::~LinuxGPIO()
{
int arg;

    /* Turn things off */
    this->off();

    gpio_close_all();

    fprintf (
        stderr,
        "LinuxGPIO::LinuxGPIO deleted\n"
    );
}

void LinuxGPIO::gpio_export(short bit)
{
    if (
        gpio_open_write_close (
            "/sys/class/gpio/export",
            (const char *)Preferences::Name("%d",bit)
        ) < 0
    ) {
        if (errno == EBUSY) {
            fprintf (
                stderr,
                "LinuxGPIO::gpio_setup gpio %d is already exported\n",
                bit
            );
        } else {
            throw runtime_error (
                (const char *)Preferences::Name (
                    "Couldn't export gpio %d",
                    bit
                )
            );
        }
    }
}

void LinuxGPIO::gpio_unexport(short bit)
{
    if (
        gpio_open_write_close (
            "/sys/class/gpio/unexport",
            (const char *)Preferences::Name("%d",bit)
        ) < 0
    ) {
        throw runtime_error (
            (const char *)Preferences::Name (
	            "Couldn't unexport gpio %d",
                bit
            )
        );
    }
}

void LinuxGPIO::gpio_set_direction(short bit, short direction)
{
    if (
        gpio_open_write_close (
            (const char *)Preferences::Name("/sys/class/gpio/gpio%d/direction",bit),
            (PIN_OUT == direction) ? "out" : "in"
        ) < 0
    ) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't set direction [%s] for gpio %d",
                (PIN_OUT == direction) ? "out" : "in",
                bit
            )
        );
    }
}

int LinuxGPIO::gpio_open(short bit)
{
    int ret = open (
        (const char *)Preferences::Name("/sys/class/gpio/gpio%d/value",bit),
        O_RDWR | O_NONBLOCK | O_SYNC
    );
    if (ret < 0) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't open value for gpio %d",
                bit
            )
        );
    }
}

int LinuxGPIO::gpio_setup (
    short reg,
    short bit,
    short invert,
    short direction
) {
    int ret = -1;

    if ((reg >= 0) && (bit >= 0)) {
        gpio_export(bit);

        try {
            gpio_set_direction(bit, direction);
            ret = gpio_open(bit);
        } catch (const char *error) {
            try {
                gpio_unexport(bit);
            } catch (const char *unexport_error) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "%s\n%s",
                        unexport_error,
                        error
                    )
                );
            }
            throw runtime_error(error);
        }
    }

    return ret;
}

void LinuxGPIO::gpio_close_all()
{
    for (std::map<int,int>::iterator it=gpiofd.begin(); it!=gpiofd.end(); ++it) {
        if (it->first >= 0) {
            if (it->second > 0) {
                close(it->second);
            }
            gpio_set_direction(it->first, PIN_INP);
            gpio_unexport(it->first);
        }
    }
}

int LinuxGPIO::gpio_open_write_close(const char *name, const char *valstr)
{
	int ret;
	int fd = open(name, O_WRONLY);

	if (fd < 0) {
		return fd;
    }

	ret = write(fd, valstr, strlen(valstr));

	close(fd);

	return ret;
}

void LinuxGPIO::set_pin_state (
    const char *name,
    short reg,
    short bit,
    short invert,
    bool  state
) {
    int fd = -1;
    
    try {
        if ((fd = gpiofd.at(bit)) <= 0) {
        }
    } catch (...) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "LinuxGPIO::set_pin_state(%s): GPIO not found",
                name
            )
        );
    }

    if (invert != 0) {
        state = !state;
    }

    if (write(fd,(state)?"1":"0",1) != 1) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "LinuxGPIO::set_pin_state(%s): write error",
                name
            )
        );
    }
}

bool LinuxGPIO::get_pin_state (
    const char *name,
    short reg,
    short bit,
    short invert
) {
    int fd = -1;
    char c;
    
    try {
        if ((fd = gpiofd.at(bit)) <= 0) {
        }
    } catch (...) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "LinuxGPIO::set_pin_state(%s): GPIO not found",
                name
            )
        );
    }

    if (read(fd,&c,1) != 1) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "LinuxGPIO::set_pin_state(%s): read error",
                name
            )
        );
    }

    return ( (c == '1') ? (invert == 0) : (invert != 0) );
}

#endif // linux && ENABLE_LINUX_GPIO

