#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

int32_t open_i2c(const char * const device_path, int32_t * const fd) {
    char dev_file[] = "/dev/i2c-10";
    uint32_t smbus_addr = 0x10;
    printf("SMBus device: %s, SMBus slave address: 0x%02X\n", device_path, smbus_addr);
    *fd = open(device_path, O_RDWR);
    if (*fd < 0) {
        perror("Function open() returned with error");
        return -1;
    }

    if (ioctl(*fd, I2C_SLAVE, smbus_addr) < 0) {
        perror("Function ioctl() returned with error");
        close(*fd);
        return -2;
    }
	return 0;
}

int32_t i2c_write_word(int32_t fd, uint16_t word) {

    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg messages[1];

    messages[0].addr = 0x20;
    messages[0].flags = 0; // Write operation (0 for write, I2C_M_RD for read)
    messages[0].len = sizeof(word);
    messages[0].buf = (uint8_t *)&word;

    data.msgs = messages;
    data.nmsgs = 1;

    return ioctl(fd, I2C_RDWR, &data);
}

uint16_t get_last_word() {
    int32_t fd = open("/tmp/rkctrl", O_CREAT | O_RDWR);
    uint16_t word = 0x0;
    read(fd, &word, sizeof(word));
    close(fd);
}

void set_last_word(uint16_t word) {
    int32_t fd = open("/tmp/rkctrl", O_CREAT | O_RDWR);
    write(fd, &word, sizeof(word));
    close(fd);
}

int32_t do_action(const char *const device_path, int32_t action) {
    printf("do_action: device_path: %s, action: %d\n", device_path, action);
    int32_t fd = -1;
    int32_t ret_val = open_i2c(device_path, &fd);

    printf("fd: %d\n", fd);
    uint16_t word = 0;
    switch (action) {
        case 0: // power on
            word = get_last_word();
            if (word == 0)
                word = 0x3;
            word |= (1 << 3);
            ret_val = i2c_write_word(fd, word);
            set_last_word(word);
            break;

        case 1: // power off
            ret_val = i2c_write_word(fd, 0);
            set_last_word(0);
            break;

        case 2: // lock sd
            word = get_last_word();
            if (word == 0)
                break;
            word |= (1 << 2);
            ret_val = i2c_write_word(fd, word);
            set_last_word(word);
            break;

        case 3: // unclock sd
            word = get_last_word();
            if (word == 0)
                break;
            word &= ~(1 << 2);
            ret_val = i2c_write_word(fd, word);
            set_last_word(word);
            break;

        case 4: // reset
            word = get_last_word();
            if (word == 0)
                break;
            word |= (1 << 1);
            sleep(1);
            word &= ~(1 << 1);
            ret_val = i2c_write_word(fd, word);
            set_last_word(word);
            break;

        case 5: // on
            word = get_last_word();
            if (word == 0)
                break;
            word |= (1 << 0);
            sleep(1);
            word &= ~(1 << 0);
            ret_val = i2c_write_word(fd, word);
            set_last_word(word);
            break;

        case 6: // off
            word = get_last_word();
            if (word == 0)
                break;
            word |= (1 << 0);
            sleep(10);
            word &= ~(1 << 0);
            ret_val = i2c_write_word(fd, word);
            set_last_word(word);
            break;
    }
    printf("ret_val: %d\n", ret_val);
    close(fd);
    return 0;
}

int32_t main(int32_t argc, char* argv[]) {
    const char *opt_string = "h?";

    const struct option long_opt[] = {
        { "pwron", no_argument, NULL, 0 },
        { "pwroff", no_argument, NULL, 1 },
        { "locksd", no_argument, NULL, 2 },
        { "unlocksd", no_argument, NULL, 3 },
        { "reset", no_argument, NULL, 4 },
        { "on", no_argument, NULL, 5 },
        { "off", no_argument, NULL, 6 },
        { "device", required_argument, NULL, 7 },
        { NULL, no_argument, NULL, 0 }
    };

    int32_t long_opt_ind = 0;
    int32_t opt_res = getopt_long(argc, (char * const*)argv, opt_string, long_opt, &long_opt_ind);
    int32_t action = -1;
    char *device_path = NULL;
    int32_t ret_val = -1;

    while( opt_res != -1 ) {
        printf("opt_res %d\n", opt_res);
        switch( opt_res ) {

            case 'h':
            case '?':
            printf("Help\n");
            break;

            case 0: /* pwron */
                printf("Switch power on \n");
                action = opt_res;
                break;

            case 1: /* pwroff */
                printf("Switch power off \n");
                action = opt_res;
                break;

            case 2: /* locksd */
                printf("SD-card locked \n");
                action = opt_res;
                break;

            case 3: /* unlocksd */
                printf("SD-card unlocked \n");
                action = opt_res;
                break;

            case 4: /* reset */
                printf("Reset button pressed\n");
                action = opt_res;
                break;

            case 5: /* on */
                printf("Switched on\n");
                action = opt_res;
                break;

            case 6: /* off */
                printf("Switched off\n");
                action = opt_res;
                break;

            case 7: /* device */
                printf("Device: %s\n", optarg);
                device_path = malloc(strlen(optarg) + 1);
                strcpy(device_path, optarg);
                break;
            default:
                printf("Unknown argument \'%c\'\n", opt_res);
                break;
        }

        if (action != -1 && device_path != NULL)
            break;

        opt_res = getopt_long(argc, (char * const*)argv, opt_string, long_opt, &long_opt_ind);
    }

    if (action != -1 && device_path != NULL)
        ret_val = do_action(device_path, action);

    free(device_path);
    return ret_val;
}
