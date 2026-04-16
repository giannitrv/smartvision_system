#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <cstdint>
#include "I2C.h"

I2C::I2C(const char* device, int address) {
    this->address = address;
    this->file = open(device, O_RDWR);
    if (this->file < 0) {
        perror("open");
    }
}

I2C::~I2C() {
    if (this->file >= 0) {
        close(this->file);
    }
}

int I2C::read(uint8_t reg, uint8_t buffer[], size_t length) {
    // Implementazione della lettura I2C
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];
    int result;

    // Primo messaggio: scrivi registro
    messages[0].addr  = this->address;
    messages[0].flags = 0;
    messages[0].len   = 1;
    messages[0].buf   = &reg;

    // Secondo messaggio: leggi valore
    messages[1].addr  = this->address;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = length;
    messages[1].buf   = buffer;

    packets.msgs = messages;
    packets.nmsgs = 2;

    result = ioctl(this->file, I2C_RDWR, &packets);
    if (result < 0) {
        perror("Errore I2C_RDWR");
        close(this->file);
    }

    return result;
}

int I2C::write(uint8_t reg, uint8_t buffer[], size_t length) {
    // Implementazione della scrittura I2C
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];
    int result;

    // Allocate buffer for register + data
    uint8_t* write_buffer = new uint8_t[length + 1];
    write_buffer[0] = reg;
    for (size_t i = 0; i < length; i++) {
        write_buffer[i + 1] = buffer[i];
    }

    // Single message: register address + data
    messages[0].addr  = this->address;
    messages[0].flags = 0;
    messages[0].len   = length + 1;
    messages[0].buf   = write_buffer;

    packets.msgs = messages;
    packets.nmsgs = 1;

    result = ioctl(this->file, I2C_RDWR, &packets);
    if (result < 0) {
        perror("Errore I2C_RDWR");
        close(this->file);
    }

    delete[] write_buffer;
    return result;
}