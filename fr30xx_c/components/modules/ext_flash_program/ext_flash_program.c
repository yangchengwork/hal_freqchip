#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fr30xx.h"
#include "ext_flash_program.h"
#include "FreeRTOS.h"
#include "crc32.h"

enum storage_type_t
{
    STORAGE_TYPE_NONE,
    STORAGE_TYPE_FLASH,
    STORAGE_TYPE_RAM,
};

enum update_param_opcode_t
{
    UP_OPCODE_GET_TYPE,         // 0
    UP_OPCODE_SEND_TYPE,        // 1
    UP_OPCODE_WRITE,            // 2
    UP_OPCODE_WRITE_ACK,        // 3
    UP_OPCODE_WRITE_RAM,        // 4
    UP_OPCODE_WRITE_RAM_ACK,    // 5
    UP_OPCODE_READ_ENABLE,      // 6
    UP_OPCODE_READ_ENABLE_ACK,  // 7
    UP_OPCODE_READ,             // 8
    UP_OPCODE_READ_ACK,         // 9
    UP_OPCODE_READ_RAM,         // a
    UP_OPCODE_READ_RAM_ACK,     // b
    UP_OPCODE_BLOCK_ERASE,      // c
    UP_OPCODE_BLOCK_ERASE_ACK,  // d
    UP_OPCODE_CHIP_ERASE,       // e
    UP_OPCODE_CHIP_ERASE_ACK,   // f
    UP_OPCODE_DISCONNECT,       // 10
    UP_OPCODE_DISCONNECT_ACK,   // 11
    UP_OPCODE_CHANGE_BANDRATE,  // 12
    UP_OPCODE_CHANGE_BANDRATE_ACK,  // 13
    UP_OPCODE_ERROR,            // 14
    UP_OPCODE_EXECUTE_CODE,     //15
    UP_OPCODE_BOOT_RAM,         //16
    UP_OPCODE_EXECUTE_CODE_END, //17
    UP_OPCODE_BOOT_RAM_ACK,     //18
    UP_OPCODE_CALC_CRC32,       //19
    UP_OPCODE_CALC_CRC32_ACK,   //1a
    UP_OPCODE_MAX,
};

enum update_cmd_proc_result_t
{
    UP_RESULT_CONTINUE,
    UP_RESULT_NORMAL_END,
    UP_RESULT_BOOT_FROM_RAM,
    UP_RESULT_RESET,
};

struct update_param_header_t
{
    uint8_t code;
    uint32_t address;
    uint16_t length;
} __attribute__((packed));

static const uint8_t ext_flash_program_boot_conn_req[] = {'F','R','E','Q','C','H','I','P'};//from embedded to pc, request
static const uint8_t ext_flash_program_boot_conn_ack[] = {'F','R','8','0','1','H','O','K'};//from pc to embedded,ack
static const uint8_t ext_flash_program_boot_conn_success[] = {'o','k'};
static const uint16_t app_boot_uart_baud_map[12] = {
    12,24,48,96,144,192,384,576,1152,2304,4608,9216
};
static uint32_t image_size;

static int ext_flash_program_serial_gets(const struct ext_flash_prog_uart_op_t *uart_op, uint8_t ms, uint8_t *data_buf, uint32_t buf_size)
{
    int i, n=0;
    uint32_t recv_size;

    for(i=0; i<ms; i++)
    {
        system_delay_us(1000);
        recv_size = uart_op->read_no_block(data_buf+n, buf_size);
        n += recv_size;
        buf_size -= recv_size;
        if(0 == buf_size)
        {
            return n;
        }
    }

    return -1;
}

static enum update_cmd_proc_result_t ext_flash_program_process_cmd(const struct ext_flash_operator_t *op, const struct ext_flash_prog_uart_op_t *uart_op, uint8_t *data, uint8_t *rsp_buffer)
{
    uint32_t req_address, req_length, rsp_length;   //req_length does not include header
    struct update_param_header_t *req_header = (struct update_param_header_t *)data;
    struct update_param_header_t *rsp_header = (void *)rsp_buffer;
    enum update_cmd_proc_result_t result = UP_RESULT_CONTINUE;

    req_address = req_header->address;
    req_length = req_header->length;

    rsp_length = sizeof(struct update_param_header_t);

    switch(req_header->code)
    {
        case UP_OPCODE_GET_TYPE:
            if(rsp_header != NULL)
            {
                rsp_header->code = UP_OPCODE_SEND_TYPE;
                rsp_header->address = op->flash_init();
            }
            break;
        case UP_OPCODE_WRITE:
            if(rsp_header != NULL)
            {
                //app_boot_save_data(req_address, req_length, data + sizeof(struct update_param_header_t));
                op->write(req_address, req_length, data + sizeof(struct update_param_header_t));
                rsp_header->code = UP_OPCODE_WRITE_ACK;
                rsp_header->address = req_address;
                rsp_header->length = req_length;
            }
            break;
        case UP_OPCODE_READ:
            rsp_length += req_length;
            if(rsp_header != NULL)
            {
                //app_boot_load_data((uint8_t *)rsp_header + sizeof(struct update_param_header_t), req_address, req_length);
                op->read(req_address, req_length, (uint8_t *)rsp_header + sizeof(struct update_param_header_t));
                rsp_header->code = UP_OPCODE_READ_ACK;
                rsp_header->address = req_address;
                rsp_header->length = req_length;
            }
            break;
        case UP_OPCODE_WRITE_RAM:
            {
                if(req_address == 0x2000f000) {
                    memcpy((void *)&image_size, data + sizeof(struct update_param_header_t), 4);
                }
//                memcpy((uint8_t *)req_address,data+sizeof(struct update_param_header_t),req_length);
                rsp_header->code = UP_OPCODE_WRITE_RAM_ACK;
                rsp_header->address = image_size;
                rsp_header->length = req_length;
            }
            break;
        case UP_OPCODE_READ_ENABLE:
            {
                rsp_header->code = UP_OPCODE_READ_ENABLE_ACK;
                rsp_header->address = req_address;
                rsp_header->length = req_length;
            }
            break;
        case UP_OPCODE_READ_RAM:
            if(rsp_header != NULL)
            {
                memcpy((uint8_t *)rsp_header + sizeof(struct update_param_header_t), (uint8_t *)req_address, req_length);
                rsp_header->code = UP_OPCODE_READ_RAM_ACK;
                rsp_header->address = req_address;
                rsp_header->length = req_length;
                rsp_length += req_length;
            }
            break;

        case UP_OPCODE_BLOCK_ERASE:
            if(rsp_header != NULL)
            {
                //app_boot_flash_sector_erase(req_address);
                op->erase(req_address, 0x1000);
                rsp_header->code = UP_OPCODE_BLOCK_ERASE_ACK;
                rsp_header->address = req_address;
                rsp_header->length = req_length;
            }
            break;
        case UP_OPCODE_CHIP_ERASE:
            if(rsp_header != NULL)
            {
                op->chip_erase();
                rsp_header->code = UP_OPCODE_CHIP_ERASE_ACK;
                rsp_header->address = req_address;
                rsp_header->length = req_length;
            }
        break;
        case UP_OPCODE_DISCONNECT:
            if(rsp_header != NULL)
            {
                rsp_header->code = UP_OPCODE_DISCONNECT_ACK;
                uart_op->write((uint8_t *)rsp_header, rsp_length);
                system_delay_us(10000);
            }
            result = (enum update_cmd_proc_result_t) (req_address & 0xFF);

            break;
        case UP_OPCODE_CHANGE_BANDRATE:
            {
                if(rsp_header != NULL)
                {
                    rsp_header->code = UP_OPCODE_CHANGE_BANDRATE_ACK;
                }
                uart_op->write((uint8_t *)rsp_header, rsp_length);
                system_delay_us(1000);
                uart_op->init(app_boot_uart_baud_map[req_address & 0xFF]*100);
            }
            break;
        case UP_OPCODE_EXECUTE_CODE:
            {
                uint32_t crc_value = 0;
                uint32_t index = 0;
                {
                    #define SINGLE_SIZE     256
                    uint8_t *buffer = pvPortMalloc(SINGLE_SIZE);
                    while(image_size) {
                        uint32_t single_size = image_size > SINGLE_SIZE ? SINGLE_SIZE : image_size;
                        op->read(index, single_size, buffer);
                        crc_value = crc32(crc_value, buffer, single_size);
                        image_size -= single_size;
                        index += single_size;
                    }
                    vPortFree(buffer);
                }
                if(rsp_header != NULL)
                {
                    rsp_header->code = UP_OPCODE_EXECUTE_CODE_END;
                    rsp_header->address = crc_value;
                }
            }
            break;
        default:
            break;
    }

    if((req_header->code != UP_OPCODE_CHANGE_BANDRATE) && (req_header->code != UP_OPCODE_DISCONNECT))
    {
        uart_op->write((uint8_t *)rsp_header, rsp_length);
    }

    return result;

}

static void ext_flash_program_host_comm_loop(const struct ext_flash_operator_t *op, const struct ext_flash_prog_uart_op_t *uart_op)
{
    enum update_cmd_proc_result_t result = UP_RESULT_CONTINUE;
    uint8_t *boot_recv_buffer = pvPortMalloc(4096);
    uint8_t *boot_send_buffer = (void *)pvPortMalloc(4096);
    struct update_param_header_t *req_header = (struct update_param_header_t *)&boot_recv_buffer[0]; //this address is useless after cpu running into this function

    while(result == UP_RESULT_CONTINUE)
    {
        uart_op->read((uint8_t *)req_header, sizeof(struct update_param_header_t));
        if((req_header->length != 0)
           &&(req_header->code != UP_OPCODE_READ)
           &&(req_header->code != UP_OPCODE_READ_RAM))
        {
            uart_op->read(((uint8_t *)req_header) + sizeof(struct update_param_header_t), req_header->length);
        }
        result = ext_flash_program_process_cmd(op, uart_op, boot_recv_buffer, boot_send_buffer);
    }
    
    vPortFree(boot_recv_buffer);
    vPortFree(boot_send_buffer);
}

void ext_flash_program(const struct ext_flash_operator_t *op, const struct ext_flash_prog_uart_op_t *uart_op)
{
    uint8_t buffer[sizeof(ext_flash_program_boot_conn_ack)];
    uint8_t do_handshake = 1;
    uint8_t retry_count = 1;

    uart_op->init(921600);

    while(retry_count) {
        uart_op->write((uint8_t *)ext_flash_program_boot_conn_req, sizeof(ext_flash_program_boot_conn_req));
        if(ext_flash_program_serial_gets(uart_op, 100, buffer, sizeof(ext_flash_program_boot_conn_ack))==sizeof(ext_flash_program_boot_conn_ack)) {
            if(memcmp(buffer, ext_flash_program_boot_conn_ack, sizeof(ext_flash_program_boot_conn_ack)) != 0) {
                do_handshake = 0;
            }
            else {
                break;
            }
        }
        else {
            do_handshake = 0;
        }

        retry_count--;
    }

    if(do_handshake) {
        uart_op->write((uint8_t *)ext_flash_program_boot_conn_success, sizeof(ext_flash_program_boot_conn_success));
        //external flash write protect
        if (op->protect_disable) {
            op->protect_disable(true);
        }
        ext_flash_program_host_comm_loop(op, uart_op);
        if (op->protect_enable) {
            op->protect_enable(false);
        }
    }
}
