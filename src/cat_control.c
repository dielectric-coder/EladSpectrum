#define _DEFAULT_SOURCE
#include "cat_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

struct cat_control {
    int fd;
    char device[256];
};

cat_control_t *cat_control_new(void) {
    cat_control_t *cat = calloc(1, sizeof(cat_control_t));
    if (!cat) return NULL;
    cat->fd = -1;
    return cat;
}

void cat_control_free(cat_control_t *cat) {
    if (!cat) return;
    cat_control_close(cat);
    free(cat);
}

int cat_control_open(cat_control_t *cat, const char *device) {
    if (!cat || !device) return -1;
    if (cat->fd >= 0) return 0;  // Already open

    // Open serial port
    cat->fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (cat->fd < 0) {
        fprintf(stderr, "CAT: Cannot open %s: %s\n", device, strerror(errno));
        return -1;
    }

    // Configure serial port: 38400 8N1
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(cat->fd, &tty) != 0) {
        fprintf(stderr, "CAT: tcgetattr failed: %s\n", strerror(errno));
        close(cat->fd);
        cat->fd = -1;
        return -1;
    }

    // Set baud rate
    cfsetispeed(&tty, B38400);
    cfsetospeed(&tty, B38400);

    // 8N1, no flow control
    tty.c_cflag &= ~PARENB;        // No parity
    tty.c_cflag &= ~CSTOPB;        // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            // 8 data bits
    tty.c_cflag &= ~CRTSCTS;       // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control

    // Raw input
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);  // No software flow control
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    // Raw output
    tty.c_oflag &= ~OPOST;

    // Read timeout: 100ms
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;  // 100ms timeout

    if (tcsetattr(cat->fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "CAT: tcsetattr failed: %s\n", strerror(errno));
        close(cat->fd);
        cat->fd = -1;
        return -1;
    }

    // Flush any pending data
    tcflush(cat->fd, TCIOFLUSH);

    strncpy(cat->device, device, sizeof(cat->device) - 1);
    fprintf(stderr, "CAT: Opened %s at 38400 baud\n", device);

    return 0;
}

void cat_control_close(cat_control_t *cat) {
    if (!cat) return;
    if (cat->fd >= 0) {
        close(cat->fd);
        cat->fd = -1;
        fprintf(stderr, "CAT: Closed\n");
    }
}

bool cat_control_is_open(cat_control_t *cat) {
    return cat && cat->fd >= 0;
}

// Send command and read response
static int cat_command(cat_control_t *cat, const char *cmd, char *response, int response_size) {
    if (!cat || cat->fd < 0) return -1;

    // Flush input buffer
    tcflush(cat->fd, TCIFLUSH);

    // Send command
    int cmd_len = strlen(cmd);
    int written = write(cat->fd, cmd, cmd_len);
    if (written != cmd_len) {
        return -1;
    }

    // Small delay for radio to process
    usleep(50000);  // 50ms

    // Read response
    int total = 0;
    int retries = 10;
    while (total < response_size - 1 && retries > 0) {
        int n = read(cat->fd, response + total, response_size - 1 - total);
        if (n > 0) {
            total += n;
            // Check if we got a complete response (ends with ;)
            if (total > 0 && response[total - 1] == ';') {
                break;
            }
        } else if (n == 0 || (n < 0 && errno == EAGAIN)) {
            retries--;
            usleep(10000);  // 10ms
        } else {
            return -1;
        }
    }

    response[total] = '\0';
    return total;
}

int cat_control_get_freq_mode(cat_control_t *cat, long *freq_hz, elad_mode_t *mode, int *vfo) {
    if (!cat || cat->fd < 0) return -1;

    char response[64];

    // Use IF command to get frequency, mode and VFO
    // Response format: IF[freq 11][step 4][rit 5][rit+/-][xit+/-][0][0][mem 2][tx][mode][vfo][scan][split]...;
    // Positions:       2-12       13-16   17-21  22      23      24 25 26-27   28  29    30   31    32
    // Example: IF00014200000000000000000000000200;
    int len = cat_command(cat, "IF;", response, sizeof(response));
    if (len < 32 || strncmp(response, "IF", 2) != 0) {
        return -1;
    }

    // Extract frequency (characters 2-12, 11 digits)
    if (freq_hz) {
        char freq_str[12];
        strncpy(freq_str, response + 2, 11);
        freq_str[11] = '\0';
        *freq_hz = atol(freq_str);
    }

    // Extract mode (character 29)
    // Kenwood modes: 1=LSB, 2=USB, 3=CW, 4=FM, 5=AM, 7=CW-R
    if (mode) {
        int kenwood_mode = response[29] - '0';
        switch (kenwood_mode) {
            case 1: *mode = ELAD_MODE_LSB; break;
            case 2: *mode = ELAD_MODE_USB; break;
            case 3: *mode = ELAD_MODE_CW; break;
            case 4: *mode = ELAD_MODE_FM; break;
            case 5: *mode = ELAD_MODE_AM; break;
            case 7: *mode = ELAD_MODE_CWR; break;
            default: *mode = ELAD_MODE_UNKNOWN; break;
        }
    }

    // Extract VFO (character 30): 0=VFO A, 1=VFO B
    if (vfo) {
        *vfo = response[30] - '0';
    }

    return 0;
}
