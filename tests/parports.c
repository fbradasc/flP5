#include <stdio.h>
#include <ieee1284.h>

int main(int argc, char argv[])
{
int i, len, daisy, capabilities;
struct parport_list ppl;
char deviceid[256];

    switch (ieee1284_find_ports(&ppl,0)) {
        case E1284_OK:
                for (i=0; i<ppl.portc; i++) {
                    printf (
                        "%s: 0x%08x - %08x - %s\n",
                        ppl.portv[i]->name,
                        ppl.portv[i]->base_addr,
                        ppl.portv[i]->hibase_addr,
                        ppl.portv[i]->filename
                    );
                    for (daisy=-1; daisy<4; daisy++) {
                        len = ieee1284_get_deviceid (
                            ppl.portv[i],
                            daisy, ~F1284_FRESH,
                            deviceid, sizeof(deviceid)-1
                        );
                        /* len = 10; */
                        if (len<=0) {
                            switch (len) {
                                case E1284_NOID:
                                    printf (
                                        "\tThe  device  did  not  provide an IEEE 1284 Device ID when interrogated\n"
                                        "\t(perhaps by the operating system if F1284_FRESH was not specified).\n"
                                    );
                                break;
                                case E1284_NOTIMPL:
                                    printf (
                                        "\tOne or more of the supplied flags is not supported in this implementation,\n"
                                        "\tor if  no  flags  were supplied then this function is not implemented for\n"
                                        "\tthis type of port or this type of system. This can also be returned if a daisy\n"
                                        "\tchain address is specified but daisy chain Device IDs are not yet supported.\n"
                                    );
                                break;
                                case E1284_NOTAVAIL:
                                    printf (
                                        "\tF1284_FRESH was specified and the library is unable to access the port to\n"
                                        "\tinterrogate the device.\n"
                                    );
                                break;
                                case E1284_NOMEM:
                                    printf("\tThere is not enough memory.\n");
                                break;
                                case E1284_INIT:
                                    printf("\tThere was a problem initializing the port.\n");
                                break;
                                case E1284_INVALIDPORT:
                                    printf("\tThe port parameter is invalid.\n");
                                break;
                                default:
                                    printf("\t???\n");
                                break;
                            }
                        } else if (len==(sizeof(deviceid)-1)) {
                            printf("\tBuffer len too short\n");
                        } else {
                            /* printf("0x%04x\n",deviceid); */
                            capabilities = CAP1284_RAW;
                            switch (ieee1284_open(ppl.portv[i],0 /*F1284_EXCL*/,&capabilities)) {
                                case E1284_OK:
                                    switch (ieee1284_claim(ppl.portv[i])) {
                                        case E1284_OK:
                                            ieee1284_release(ppl.portv[i]);
                                        break;
                                        case E1284_NOMEM:
                                            printf (
                                                "\t\t\tThere is not enough memory.\n"
                                            );
                                        break;
                                        case E1284_INVALIDPORT:
                                            printf (
                                                "\t\t\tThe port parameter is invalid (for instance, it might not have\n"
                                                "\t\t\tbeen opened yet).\n"
                                            );
                                        break;
                                        case E1284_SYS:
                                            printf (
                                                "\t\t\tThere  was a problem at the operating system level. The global\n"
                                                "\t\t\tvariable errno has been set appropriately.\n"
                                            );
                                        break;
                                        default:
                                            printf("\t\t\t???\n");
                                        break;
                                    }
                                    switch (ieee1284_close(ppl.portv[i])) {
                                        case E1284_OK:
                                        break;
                                        case E1284_INVALIDPORT:
                                            printf (
                                                "\t\t\t\tThe port parameter is invalid (perhaps it is not open,\n"
                                                "\t\t\t\tfor instance).\n"
                                            );
                                        break;
                                        case E1284_SYS:
                                            printf (
                                                "\t\t\t\tThere  was a problem at the operating system level. The global\n"
                                                "\t\t\t\tvariable errno has been set appropriately.\n"
                                            );
                                        break;
                                        default:
                                            printf("\t\t\t\t???\n");
                                        break;
                                    }
                                break;
                                case E1284_INIT:
                                    printf (
                                        "\t\tThere was a problem during port initialization. This could be because\n"
                                        "\t\tanother driver  has  opened the port exclusively, or some other reason.\n"
                                    );
                                break;
                                case E1284_NOMEM:
                                    printf (
                                        "\t\tThere is not enough memory.\n"
                                    );
                                break;
                                case E1284_NOTAVAIL:
                                    printf (
                                        "\t\tOne or more of the supplied flags is not supported by this type of port.\n"
                                    );
                                break;
                                case E1284_INVALIDPORT:
                                    printf (
                                        "\t\tThe port parameter is invalid (for instance, the port may already be open).\n"
                                    );
                                break;
                                case E1284_SYS:
                                    printf (
                                        "\t\tThere  was a problem at the operating system level. The global variable\n"
                                        "\t\terrno thas been set appropriately.\n"
                                    );
                                break;
                                default:
                                    printf("\t\t???\n");
                                break;
                            }
                        }
                    }
                }
                ieee1284_free_ports(&ppl);
            break;
        case E1284_NOMEM:
                printf ("There is not enough memory available.\n");
            break;
        case E1284_NOTIMPL:
                printf ("One or more of the supplied flags is not supported in this implementation.\n");
            break;
        default:
                printf ("???\n");
            break;
    }
    return 0;
}
