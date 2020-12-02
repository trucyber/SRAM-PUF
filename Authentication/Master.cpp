/*
* Created by Abel Gomez 04/17/2020
* the program simulates a master device (RTU) in the modbus protocol
* the master pull information from a temperature sensor (or slave).
*/


#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include "stdio.h"
#include "string.h"

#define HASH_LEN 32
#define MODBUS_PORT 1502
#define CHALLENGE_SIZE 256
#define SLAVE_IP "127.0.0.1"

/*
* Slave - Mode of Operation, 4 bits
*/
#define MODE 4
#define WRITE_C    0b0000
#define WRITE_TS   0b0001
#define WRITE_HMAC 0b0010
#define READ_DATA  0b0011
#define READ_HMAC  0b0100

modbus_t *context;

uint8_t *hmac;
uint8_t *bits;
uint8_t *slave_mode;
uint8_t *master_hmac;
uint8_t temperature = 0;

uint16_t *challenge;
uint16_t *registers;

/* 
* Function Prototypes 
*/
int read_hmac();
int setMode(int);
int initialize();
int read_sensor();
int write_ts(uint32_t);
int write_master_hmac();
int write_challenge(uint16_t *);
void get_master_hmac(uint32_t ts);

int initialize() {
    /* Set response timeout*/
    modbus_set_byte_timeout(context, 0, 0);
    modbus_set_response_timeout(context, 500, 0);

    /* Allocate and initialize the memory to store the status */
    bits = (uint8_t *) malloc(MODBUS_MAX_READ_BITS * sizeof(uint8_t));
    if (bits == NULL) {
        fprintf(stderr, "Failed to allocated memory");
        return -1;
    }
    memset(bits, 0, MODBUS_MAX_READ_BITS * sizeof(uint8_t));
    
    /* Allocate and initialize the memory to store master_hmac */
    master_hmac = (uint8_t *) malloc(HASH_LEN * sizeof(uint8_t));
    if (master_hmac == NULL) {
        fprintf(stderr, "Failed to allocated memory");
        return -1;
    }
    memset(master_hmac, 0, HASH_LEN * sizeof(uint8_t));

    /* Allocate and initialize the memory to store hmac */
    hmac = (uint8_t *) malloc(HASH_LEN * sizeof(uint8_t));
    if (hmac == NULL) {
        fprintf(stderr, "Failed to allocated memory");
        return -1;
    }
    memset(hmac, 0, HASH_LEN * sizeof(uint8_t));

        /* Allocate and initialize the memory to store the registers */
    registers = (uint16_t *) malloc(MODBUS_MAX_READ_REGISTERS * sizeof(uint16_t));
    if (registers == NULL) {
        fprintf(stderr, "Failed to allocated memory");
        return -1;
    }
    memset(registers, 0, MODBUS_MAX_READ_REGISTERS * sizeof(uint16_t));

    /* Allocate and initialize the memory to store the c1 */
    challenge = (uint16_t *) malloc(CHALLENGE_SIZE * sizeof(uint16_t));
    if (challenge == NULL) {
        fprintf(stderr, "Failed to allocated memory");
        return -1;
    }
    memset(challenge, 0, CHALLENGE_SIZE * sizeof(uint16_t));

    /* Allocate and initialize the memory to store the mode */
    slave_mode = (uint8_t *) malloc(MODE * sizeof(uint8_t));
    if (slave_mode == NULL) {
        fprintf(stderr, "Failed to allocated memory");
        return -1;
    }
    memset(slave_mode, 0, MODE * sizeof(uint8_t));

    return 0;
}

int setMode(int mode) {
    int rc = 0;

    /* Set slave mode */
    switch (mode) {
        case WRITE_C:
            slave_mode[0] = 0;
            slave_mode[1] = 0;
            slave_mode[2] = 0;
            slave_mode[3] = 0;
            rc = modbus_write_bits(context, 0, 4, slave_mode);
            if (rc == -1) {
                fprintf(stderr, "%s\n", modbus_strerror(errno));
                return -1;
            }
            break;
        case WRITE_TS:
            slave_mode[0] = 0;
            slave_mode[1] = 0;
            slave_mode[2] = 0;
            slave_mode[3] = 1;
            rc = modbus_write_bits(context, 0, 4, slave_mode);
            if (rc == -1) {
                fprintf(stderr, "%s\n", modbus_strerror(errno));
                return -1;
            }
            break;
        case WRITE_HMAC:
            slave_mode[0] = 0;
            slave_mode[1] = 0;
            slave_mode[2] = 1;
            slave_mode[3] = 0;
            rc = modbus_write_bits(context, 0, 4, slave_mode);
            if (rc == -1) {
                fprintf(stderr, "%s\n", modbus_strerror(errno));
                return -1;
            }
            break;
        case READ_DATA:
            slave_mode[0] = 0;
            slave_mode[1] = 0;
            slave_mode[2] = 1;
            slave_mode[3] = 1;
            rc = modbus_write_bits(context, 0, 4, slave_mode);
            if (rc == -1) {
                fprintf(stderr, "%s\n", modbus_strerror(errno));
                return -1;
            }
            break;
        case READ_HMAC:
            slave_mode[0] = 0;
            slave_mode[1] = 1;
            slave_mode[2] = 0;
            slave_mode[3] = 0;
            rc = modbus_write_bits(context, 0, 4, slave_mode);
            if (rc == -1) {
                fprintf(stderr, "%s\n", modbus_strerror(errno));
                return -1;
            }
            break;
        default:
            printf("Incorrect Mode\n");
            return -1;
    }
    return 0;
}

int write_challenge(uint16_t * challenge) {
    int rc = 0;
    int section_no = 0;
    int remainder_bit = 0;

    section_no = (int) ceil(CHALLENGE_SIZE/MODBUS_MAX_WRITE_REGISTERS);

    for (int i = 0; i < section_no; i++) {
        /* send challenge to slave */
        memcpy(registers, &challenge[i*MODBUS_MAX_WRITE_REGISTERS], MODBUS_MAX_WRITE_REGISTERS*sizeof(uint16_t));
        
        rc = modbus_write_registers(context, 0, MODBUS_MAX_WRITE_REGISTERS, registers);
        if (rc == -1) {
            fprintf(stderr, "%s\n", modbus_strerror(errno));
            return -1;
        }
    }

    remainder_bit = CHALLENGE_SIZE - MODBUS_MAX_WRITE_REGISTERS * (int) ceil(CHALLENGE_SIZE/MODBUS_MAX_WRITE_REGISTERS);
    if (remainder_bit > 0) {
        /* send challenge to slave */
        memcpy(registers, &challenge[section_no*MODBUS_MAX_WRITE_REGISTERS], MODBUS_MAX_WRITE_REGISTERS*sizeof(uint16_t));
        
        rc = modbus_write_registers(context, 0, remainder_bit, registers);
        if (rc == -1) {
            fprintf(stderr, "%s\n", modbus_strerror(errno));
            return -1;
        }
    }

    return 0;
}

int write_ts(uint32_t timestamp) {
    int rc = 0;
    uint16_t ts_array[2];
    
    memset(ts_array, 0, 2 * sizeof(uint16_t));
    memcpy(ts_array, &timestamp, 4);
    
    rc = modbus_write_registers(context, 0, 2, ts_array);
    if (rc == -1) {
        fprintf(stderr, "%s\n", modbus_strerror(errno));
        return -1;
    }
    
    return 0;
}

int write_master_hmac() {
    int rc = 0;

    memcpy(registers, master_hmac, HASH_LEN * sizeof(uint8_t));
    
    rc = modbus_write_registers(context, 0, HASH_LEN, registers);
    if (rc == -1) {
        fprintf(stderr, "%s\n", modbus_strerror(errno));
        return -1;
    }

    return 0;
}

int read_sensor() {
    int rc = 0;

    rc = modbus_read_registers(context, 0, 1, registers);
    if (rc == -1) {
        fprintf(stderr, "%s\n", modbus_strerror(errno));
        return -1;
    }

    memcpy(&temperature, registers, sizeof(uint8_t));

    printf("%d-", temperature);
    // printf("Temperature read from sensor: %d\n", temperature);
    return 0;
}

int read_hmac() {
    int rc = 0;

    rc = modbus_read_registers(context, 0, HASH_LEN, registers);
    if (rc == -1) {
        fprintf(stderr, "%s\n", modbus_strerror(errno));
        return -1;
    }

    memcpy(hmac, registers, HASH_LEN*sizeof(uint8_t));

    for (size_t index = 0; index < 32; index ++) {
        printf("%02x", hmac[index]);
    }

    return 0;
}

int main(int argc, char **argv){
    size_t index = 0;

    // check # arguments
    if (argc < 3) {
        printf("Incorect number of argument!!!!\n");
        printf("use: master <challenge>, <hmac>, <ts>\n");
        exit(EXIT_FAILURE);
    }

    /* Initialize variables */
    if (initialize() != 0) {
        printf("Failed to allocated memory\n");
        exit(EXIT_FAILURE);
    }

    // read challenge
    int NoLocation = 0;
    char *location = strtok(argv[1], ",");
    while (location != NULL) {
        // printf("%s ", location);
        challenge[NoLocation] = atoi(location);
        location = strtok(NULL, ",");
        NoLocation++;
    }
    // for (index = 0; index < NoLocation; index++) {
    //     printf("%d ", challenge[index]);
    // }

    // read hmac (h1)
    size_t lenH1 = strlen(argv[2]);
    for (index = 0; index < (lenH1 / 2); index ++) {
        sscanf(argv[2] + 2*index, "%02x", &master_hmac[index]);
        // printf("%02x ", master_hmac[index]);
    }

    // read ts'
    uint32_t timestamp;
    char *ptr;
    timestamp = strtoul(argv[3], &ptr, 10);

    /* Connect to slave TODO: change communication link and add multiple slaves*/
    context = modbus_new_tcp(SLAVE_IP, MODBUS_PORT);

    if (modbus_connect(context) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(context);
        exit(EXIT_FAILURE);
    }
    
    /* Set slave mode */
    if (setMode(WRITE_C) != 0) {
        printf("Failed to set slave mode\n");
        exit(EXIT_FAILURE);
    }

    /* Write challenge */
    if (write_challenge(challenge) != 0) {
        printf("Failed to write H\n");
        exit(EXIT_FAILURE);
    }
        
    /* Set slave mode */
    if (setMode(WRITE_TS) != 0) {
        printf("Failed to set slave mode\n");
        exit(EXIT_FAILURE);
    }

    /* write TS */
    if (write_ts(timestamp) != 0) {
        printf("Failed to write challenge\n");
        exit(EXIT_FAILURE);
    }

    /* Set slave mode */
    if (setMode(WRITE_HMAC) != 0) {
        printf("Failed to set slave mode\n");
        exit(EXIT_FAILURE);
    }

    /* write master hmac */
    if (write_master_hmac() != 0) {
        printf("Failed to write challenge\n");
        exit(EXIT_FAILURE);
    }
 
    /* Set slave mode */
    if (setMode(READ_DATA) != 0) {
        printf("Failed to set slave mode\n");
        exit(EXIT_FAILURE);
    }
        
    /* read sensor */
    if (read_sensor() != 0) {
        printf("Failed to read sensor\n");
        exit(EXIT_FAILURE);
    }
        
    /* Set slave mode */
    if (setMode(READ_HMAC) != 0) {
        printf("Failed to set slave mode\n");
        exit(EXIT_FAILURE);
    }

    /* read sensor data */
    if (read_hmac() != 0) {
        printf("Failed to verified data integrity\n");
        exit(EXIT_FAILURE);
    }
   
     /* Free the memory */
    free(bits);
    free(registers);

    /* Close the connection */
    modbus_close(context);
    modbus_free(context);

    return 0;
}
