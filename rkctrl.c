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


static int32_t i2c_smbus_access(int file, char read_write, uint8_t command, int32_t size, union i2c_smbus_data *data) {
    struct i2c_smbus_ioctl_data args;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;
    return ioctl(file, I2C_SMBUS, &args);
}

//************************************************

static int32_t i2c_smbus_read_byte(int32_t file) {
    union i2c_smbus_data data;
    if (i2c_smbus_access(file, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data))
        return -1;
    else
        return 0x0FF & data.byte;
}

static int32_t i2c_smbus_read_byte_data(int32_t file, uint8_t command) {
    union i2c_smbus_data data;
    if (i2c_smbus_access(file, I2C_SMBUS_READ, command, I2C_SMBUS_BYTE_DATA, &data))
        return -1;
    else
        return 0x0FF & data.byte;
}

static int32_t i2c_smbus_read_word_data(int32_t file, uint8_t command) {
    union i2c_smbus_data data;
    if (i2c_smbus_access(file, I2C_SMBUS_READ, command, I2C_SMBUS_WORD_DATA, &data))
        return -1;
    else
        return 0x0FFFF & data.word;
}

static int32_t i2c_smbus_read_block_data(int32_t file, uint8_t command, uint8_t *values) {
    union i2c_smbus_data data;
    int32_t i;
    if (i2c_smbus_access(file, I2C_SMBUS_READ, command, I2C_SMBUS_BLOCK_DATA, &data))
        return -1;
    else {
        for (i = 1; i <= data.block[0]; i++)
            values[i-1] = data.block[i];
        return data.block[0];
    }
}

//*************************************************

static int32_t i2c_smbus_write_quick(int32_t file, uint8_t value) {
    return i2c_smbus_access(file, value, 0, I2C_SMBUS_QUICK, NULL);
}

static int32_t i2c_smbus_write_byte(int32_t file, uint8_t value) {
    return i2c_smbus_access(file, I2C_SMBUS_WRITE, value, I2C_SMBUS_BYTE,NULL);
}

static int32_t i2c_smbus_write_byte_data(int32_t file, uint8_t command, uint8_t value) {
    union i2c_smbus_data data;
    data.byte = value;
    return i2c_smbus_access(file, I2C_SMBUS_WRITE, command, I2C_SMBUS_BYTE_DATA, &data);
}

static int32_t i2c_smbus_write_word_data(int32_t file, uint8_t command, uint16_t value) {
    union i2c_smbus_data data;
    data.word = value;
    return i2c_smbus_access(file, I2C_SMBUS_WRITE, command, I2C_SMBUS_WORD_DATA, &data);
}

static int32_t i2c_smbus_write_block_data(int32_t file, uint8_t command, uint8_t length, const uint8_t *values) {
    union i2c_smbus_data data;
    int32_t i;
    if (length > 32)
        length = 32;
    for (i = 1; i <= length; i++)
        data.block[i] = values[i-1];
    data.block[0] = length;
    return i2c_smbus_access(file, I2C_SMBUS_WRITE, command, I2C_SMBUS_BLOCK_DATA, &data);
}

//************************************************

int32_t open_i2c(int32_t *fd) {
    char dev_file[] = "/dev/i2c-10";
    uint32_t smbus_addr = 0x10;
    printf("SMBus device: %s, SMBus slave address: 0x%02X\n", dev_file, smbus_addr);
    *fd = open(dev_file, O_RDWR);
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

int32_t read_register(int32_t fd, uint16_t reg_addr, uint32_t *preg_value) {
    uint8_t buffer[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    buffer[0] = reg_addr & 0xFF;
    buffer[1] = reg_addr >> 8;
    int32_t ret_val = i2c_smbus_write_block_data(fd, 0x02, 0x2, (uint8_t *)&buffer);
//    printf("ret_val: %d\n", ret_val);
    if (ret_val < 0) {
        perror("Function ioctl() returned with error");
        close(fd);
        return -1;
    }

    ret_val = i2c_smbus_read_block_data(fd, 0x01, (uint8_t *)&buffer);
//    printf("ret_val: %d\n", ret_val);
    if (ret_val < 0) {
        perror("Function ioctl() returned with error");
        return -2;
    }
    *preg_value = buffer[5] << 24 | buffer[4] << 16 | buffer[3] << 8 | buffer[2];
    return 0;
}

int32_t write_register(int32_t fd, uint16_t reg_addr, uint32_t reg_value) {
    uint8_t buffer[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    buffer[0] = reg_addr & 0xFF;
    buffer[1] = reg_addr >> 8;
    buffer[2] = (reg_value >> 0) & 0xFF;
    buffer[3] = (reg_value >> 8) & 0xFF;
    buffer[4] = (reg_value >> 16) & 0xFF;
    buffer[5] = (reg_value >> 24) & 0xFF;
    int32_t ret_val = i2c_smbus_write_block_data(fd, 0x07, 0x6, (uint8_t *)&buffer);
//    printf("ret_val: %d\n", ret_val);
    if (ret_val < 0) {
        perror("Function ioctl() returned with error");
        return -1;
    }

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
        { "sw", no_argument, NULL, 5 },
        { NULL, no_argument, NULL, 0 }
    };
    int32_t long_opt_ind = 0;
    int32_t opt_res = getopt_long(argc, (char * const*)argv, opt_string, long_opt, &long_opt_ind);
    while( opt_res != -1 ) {
        printf("opt_res %d\n", opt_res);
        switch( opt_res ) {

            case 'h':
            case '?':
            printf("Help\n");
            break;

            case 0: /* pwron */
                printf("Switch power on \n");
                break;

            case 1: /* pwroff */
                printf("Switch power off \n");
                break;

            case 2: /* locksd */
                printf("SD-card locked \n");
                break;

            case 3: /* unlocksd */
                printf("SD-card unlocked \n");
                break;

            case 4: /* reset */
                printf("Reset button pressed\n");
                break;

            case 5: /* sw */
                printf("Power button pressed\n");
                break;

            default:
                printf("Unknown argument \'%c\'\n", opt_res);
                break;
        }
        opt_res = getopt_long(argc, (char * const*)argv, opt_string, long_opt, &long_opt_ind);
    }
    return 0;
}
