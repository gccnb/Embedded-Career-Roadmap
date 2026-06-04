/*
 * Teaching purpose:
 * - Open a Linux serial device such as /dev/ttyUSB0.
 * - Configure 9600 8N1 raw mode with termios.
 * - Send one teaching Modbus-like request frame and read bytes back.
 *
 * This file does not depend on a specific chip library or real device.
 * Real wiring, A/B terminal definition, baud rate and protocol details
 * must be checked against manuals, schematics and measurements.
 */

#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

static int configure_serial(int fd, speed_t baud)
{
    struct termios opt;

    if (tcgetattr(fd, &opt) != 0) {
        perror("tcgetattr");
        return -1;
    }

    cfmakeraw(&opt);
    cfsetispeed(&opt, baud);
    cfsetospeed(&opt, baud);

    opt.c_cflag |= (CLOCAL | CREAD);
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    opt.c_cflag &= ~PARENB;
    opt.c_cflag &= ~CSTOPB;
    opt.c_cflag &= ~CRTSCTS;

    opt.c_cc[VMIN] = 0;
    opt.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &opt) != 0) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}

static void print_hex(const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

static int read_with_timeout(int fd, uint8_t *buffer, int maxLen, int timeoutMs)
{
    fd_set readSet;
    struct timeval timeout;
    int ret;

    FD_ZERO(&readSet);
    FD_SET(fd, &readSet);

    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    ret = select(fd + 1, &readSet, NULL, NULL, &timeout);
    if (ret < 0) {
        perror("select");
        return -1;
    }

    if (ret == 0) {
        return 0;
    }

    return (int)read(fd, buffer, (size_t)maxLen);
}

int main(int argc, char *argv[])
{
    const char *device = "/dev/ttyUSB0";
    uint8_t request[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
    uint8_t rx[128];
    int fd;
    int n;

    if (argc >= 2) {
        device = argv[1];
    }

    fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "open %s failed: %s\n", device, strerror(errno));
        return 1;
    }

    if (configure_serial(fd, B9600) != 0) {
        close(fd);
        return 1;
    }

    printf("TX: ");
    print_hex(request, (int)sizeof(request));

    if (write(fd, request, sizeof(request)) != (ssize_t)sizeof(request)) {
        perror("write");
        close(fd);
        return 1;
    }

    tcdrain(fd);

    n = read_with_timeout(fd, rx, (int)sizeof(rx), 500);
    if (n < 0) {
        close(fd);
        return 1;
    }

    if (n == 0) {
        printf("RX timeout. Check device, baud rate, wiring and protocol.\n");
    } else {
        printf("RX: ");
        print_hex(rx, n);
    }

    close(fd);
    return 0;
}
