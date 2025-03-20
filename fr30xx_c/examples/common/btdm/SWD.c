/*
  ******************************************************************************
  * @file    SWD.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2023
  * @brief   SWD module Demo.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "co_util.h"

#include "fr30xx.h"

#include "SWD.h"

#define PMU_SWD_IE          0x43
#define PMU_SWD_DATA        0x49
#define PMU_SWD_DIR         0x4b
#define PMU_SWD_IOMUX       0x5b
#define PMU_SWD_PULL_EN     0x45
#define PMU_SWD_PULL_SEL    0x47
//#define PMU_SWD_IE      0x42
//#define PMU_SWD_DATA    0x48
//#define PMU_SWD_DIR     0x4a
//#define PMU_SWD_IOMUX   0x59

#define SWD_CLK_H       0x01
#define SWD_CLK_L       0x02
#define SWD_DATA_H      0x02
#define SWD_DATA_L      0x01

static uint8_t SWD_DATA,SWD_CLK;

#define ool_write_ram(addr, data)       frspim_wr_ram(FR_SPI_PMU_CHAN,(addr),1, (data))
#define ool_read_ram(addr)              (uint8_t)frspim_rd_ram(FR_SPI_PMU_CHAN,(addr),1)

/*---------------------------------- Porting ----------------------------------------*/

#define RAM_CODE __RAM_CODE


#define SWD_CLK_SET_H()    {SWD_CLK = SWD_CLK_H; \
                            ool_write_ram(PMU_SWD_DATA,SWD_DATA | SWD_CLK);}
#define SWD_CLK_SET_L()    {SWD_CLK = SWD_CLK_L; \
                            ool_write_ram(PMU_SWD_DATA,SWD_DATA & SWD_CLK);}

#define SWD_IO_SET_H()     {SWD_DATA = SWD_DATA_H; \
                            ool_write_ram(PMU_SWD_DATA,SWD_CLK | SWD_DATA);}
#define SWD_IO_SET_L()     {SWD_DATA = SWD_DATA_L; \
                            ool_write_ram(PMU_SWD_DATA,SWD_CLK & SWD_DATA);}
#define SWD_IO_IN()        (ool_read_ram(PMU_SWD_DATA) & 0x02)

#define SWD_IO_OUT_ENABLE()     (ool_write_ram(PMU_SWD_DIR,0x00))
#define SWD_IO_OUT_DISABLE()    (ool_write_ram(PMU_SWD_DIR,0x02))

void SWD_IO_init(void)
{
    ool_write_ram(PMU_SWD_PULL_EN, 0x03);
    ool_write_ram(PMU_SWD_PULL_SEL, 0x00);
    ool_write_ram(PMU_SWD_IE, 0x03);
    ool_write_ram(PMU_SWD_DIR, 0x00);
    ool_write_ram(PMU_SWD_DATA, 0x03);
    ool_write_ram(PMU_SWD_IOMUX, 0x00);

    SWD_CLK_SET_L();
    SWD_IO_SET_L();
}
/*---------------------------------- Porting end ----------------------------------------*/


#define SW_CLOCK_CYCLE()                \
    SWD_CLK_SET_H();                    \
    SWD_DELAY();                        \
    SWD_CLK_SET_L();                    \
    SWD_DELAY()

#define SW_WRITE_BIT_0()                \
    SWD_IO_SET_L();                     \
    SWD_DELAY();                        \
    SWD_DELAY();                        \
    SWD_CLK_SET_H();                    \
    SWD_DELAY();                        \
    SWD_DELAY();                        \
    SWD_CLK_SET_L()

#define SW_WRITE_BIT_1()                \
    SWD_IO_SET_H();                     \
    SWD_DELAY();                        \
    SWD_DELAY();                        \
    SWD_CLK_SET_H();                    \
    SWD_DELAY();                        \
    SWD_DELAY();                        \
    SWD_CLK_SET_L()


#define SW_READ_BIT(BIT)                \
    SWD_CLK_SET_H();                    \
    SWD_DELAY();                        \
    SWD_DELAY();                        \
    SWD_CLK_SET_L();                    \
    SWD_DELAY();                        \
    SWD_DELAY();                        \
    BIT = SWD_IO_IN()


RAM_CODE static inline void SW_WRITE_BIT(uint32_t BIT)
{
    switch (BIT)
    {
        case 0:
        {
            SW_WRITE_BIT_0();
        }break;

        case 1:
        {
            SW_WRITE_BIT_1();
        }break;

        default:break;
    }
}

RAM_CODE int SWD_TransferFunction(uint32_t request, uint32_t *data, uint32_t Trailing)
{
    int i;
    uint32_t W_Parity;

    uint32_t ACK;
    bool BitBuffer[33];
    bool ACKBuffer[3];

    bool APnDP  = (request >> 0) & 1;
    bool RnW    = (request >> 1) & 1;
    bool A2     = (request >> 2) & 1;
    bool A3     = (request >> 3) & 1;
    bool parity = (APnDP + RnW + A2 + A3) & 0x01;

    if (RnW == 0)
    {
        /* Prepare data */
        W_Parity = 0;
        for (i = 0; i < 32; i++)
        {
            BitBuffer[i] = (*data >> i) & 1;
            W_Parity += BitBuffer[i];
        }

        W_Parity &= 0x01;
        BitBuffer[32] = W_Parity;
    }

    SWD_IO_OUT_ENABLE();
    /* Packet Request */ 
    SW_WRITE_BIT(1U);                     /* Start Bit */
    SW_WRITE_BIT(APnDP);                  /* APnDP Bit */
    SW_WRITE_BIT(RnW);                    /* RnW Bit */
    SW_WRITE_BIT(A2);                     /* A2 Bit */
    SW_WRITE_BIT(A3);                     /* A3 Bit */
    SW_WRITE_BIT(parity);                 /* parity Bit */
    SW_WRITE_BIT(0U);                     /* Stop Bit */
    SW_WRITE_BIT(1U);                     /* Park Bit */

    /* Turnaround */
    SWD_IO_OUT_DISABLE();
    __NOP();__NOP();__NOP();__NOP();__NOP();SWD_DELAY();
    SWD_CLK_SET_H();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();


    /* Acknowledge response */
    SW_READ_BIT(ACKBuffer[0]);
    SW_READ_BIT(ACKBuffer[1]);
    SW_READ_BIT(ACKBuffer[2]);
    ACK = (ACKBuffer[0] << 2) | (ACKBuffer[1] << 1) | ACKBuffer[2];

    /* OK response */ 
    if (ACK == DAP_TRANSFER_OK)
    {
        /* Data transfer */
        if (RnW == 1)
        {
            /* Read data */

            /* Read RDATA[0:31] */
            SW_READ_BIT(BitBuffer[0]);
            SW_READ_BIT(BitBuffer[1]);
            SW_READ_BIT(BitBuffer[2]);
            SW_READ_BIT(BitBuffer[3]);
            SW_READ_BIT(BitBuffer[4]);
            SW_READ_BIT(BitBuffer[5]);
            SW_READ_BIT(BitBuffer[6]);
            SW_READ_BIT(BitBuffer[7]);
            SW_READ_BIT(BitBuffer[8]);
            SW_READ_BIT(BitBuffer[9]);
            SW_READ_BIT(BitBuffer[10]);
            SW_READ_BIT(BitBuffer[11]);
            SW_READ_BIT(BitBuffer[12]);
            SW_READ_BIT(BitBuffer[13]);
            SW_READ_BIT(BitBuffer[14]);
            SW_READ_BIT(BitBuffer[15]);
            SW_READ_BIT(BitBuffer[16]);
            SW_READ_BIT(BitBuffer[17]);
            SW_READ_BIT(BitBuffer[18]);
            SW_READ_BIT(BitBuffer[19]);
            SW_READ_BIT(BitBuffer[20]);
            SW_READ_BIT(BitBuffer[21]);
            SW_READ_BIT(BitBuffer[22]);
            SW_READ_BIT(BitBuffer[23]);
            SW_READ_BIT(BitBuffer[24]);
            SW_READ_BIT(BitBuffer[25]);
            SW_READ_BIT(BitBuffer[26]);
            SW_READ_BIT(BitBuffer[27]);
            SW_READ_BIT(BitBuffer[28]);
            SW_READ_BIT(BitBuffer[29]);
            SW_READ_BIT(BitBuffer[30]);
            SW_READ_BIT(BitBuffer[31]);

            /* Read Parity */
            SW_READ_BIT(BitBuffer[32]);

            /* Trailing  */
            __NOP();__NOP();__NOP();__NOP();__NOP();
            for (i = 0; i < Trailing; i++)
            {
                SW_CLOCK_CYCLE();
            }

            *data = 0;
            for (i = 0; i < 32; i++)
            {
                *data |= (BitBuffer[i] << i);
            }
        }
        else
        {
            /* Write data */

            /* Turnaround */
            SWD_CLK_SET_H();
            __NOP();__NOP();__NOP();__NOP();__NOP();SWD_DELAY();
            SWD_CLK_SET_L();
            __NOP();__NOP();__NOP();__NOP();__NOP();SWD_DELAY();
            SWD_CLK_SET_H();
            __NOP();__NOP();__NOP();__NOP();__NOP();SWD_DELAY();
            SWD_CLK_SET_L();
            __NOP();__NOP();__NOP();__NOP();__NOP();SWD_DELAY();

            SWD_IO_OUT_ENABLE();

            /* Write WDATA[0:31] */
            SW_WRITE_BIT(BitBuffer[0]);
            SW_WRITE_BIT(BitBuffer[1]);
            SW_WRITE_BIT(BitBuffer[2]);
            SW_WRITE_BIT(BitBuffer[3]);
            SW_WRITE_BIT(BitBuffer[4]);
            SW_WRITE_BIT(BitBuffer[5]);
            SW_WRITE_BIT(BitBuffer[6]);
            SW_WRITE_BIT(BitBuffer[7]);
            SW_WRITE_BIT(BitBuffer[8]);
            SW_WRITE_BIT(BitBuffer[9]);
            SW_WRITE_BIT(BitBuffer[10]);
            SW_WRITE_BIT(BitBuffer[11]);
            SW_WRITE_BIT(BitBuffer[12]);
            SW_WRITE_BIT(BitBuffer[13]);
            SW_WRITE_BIT(BitBuffer[14]);
            SW_WRITE_BIT(BitBuffer[15]);
            SW_WRITE_BIT(BitBuffer[16]);
            SW_WRITE_BIT(BitBuffer[17]);
            SW_WRITE_BIT(BitBuffer[18]);
            SW_WRITE_BIT(BitBuffer[19]);
            SW_WRITE_BIT(BitBuffer[20]);
            SW_WRITE_BIT(BitBuffer[21]);
            SW_WRITE_BIT(BitBuffer[22]);
            SW_WRITE_BIT(BitBuffer[23]);
            SW_WRITE_BIT(BitBuffer[24]);
            SW_WRITE_BIT(BitBuffer[25]);
            SW_WRITE_BIT(BitBuffer[26]);
            SW_WRITE_BIT(BitBuffer[27]);
            SW_WRITE_BIT(BitBuffer[28]);
            SW_WRITE_BIT(BitBuffer[29]);
            SW_WRITE_BIT(BitBuffer[30]);
            SW_WRITE_BIT(BitBuffer[31]);

            /* Write Parity Bit */
            SW_WRITE_BIT(BitBuffer[32]);

            SWD_IO_OUT_DISABLE();
            __NOP();__NOP();__NOP();__NOP();__NOP();
            __NOP();__NOP();__NOP();__NOP();__NOP();
            __NOP();__NOP();__NOP();__NOP();__NOP();
            __NOP();__NOP();__NOP();__NOP();__NOP();
            /* Trailing  */
            for (i = 0; i < Trailing; i++)
            {
                SW_CLOCK_CYCLE();
            }
        }
    }
    else
    {
        /* WAIT or FAULT response */
        if (ACK == DAP_TRANSFER_WAIT)
        {
            SW_CLOCK_CYCLE();
            SW_CLOCK_CYCLE();
            return -1;
        }
        else
        {
            return -2;
        }
    }

    return 0;
}

RAM_CODE void SWD_Line_Reset(uint32_t fu32_CNT)
{
    int i;
    SWD_IO_OUT_ENABLE();
    
    SWD_IO_SET_H();
    for (i = 0; i < fu32_CNT; i++)
    {
        SW_CLOCK_CYCLE();
    }
    SWD_IO_SET_L();

    for (int i = 0; i < 16; i++)
    {
        SW_CLOCK_CYCLE();
    }
}

RAM_CODE void SWD_Connect(void)
{
    uint32_t Data;

    /* Line Reset */
    SWD_Line_Reset(56);
    SWD_Line_Reset(59);
    SWD_Line_Reset(59);
    for (int i = 0; i < 16; i++)
    {
        SW_CLOCK_CYCLE();
    }

    /* Read IDCODE */
    SWD_TransferFunction(REQ_DP|REQ_R|REQ_ADDR_0, &Data, 6); co_delay_10us(1);
}

RAM_CODE void SWD_Enable_Debug(void)
{
    uint32_t Data;

    /* Write Abort */
    Data = 0x0000001E;
    SWD_TransferFunction(REQ_DP|REQ_W|REQ_ADDR_0, &Data, 7); co_delay_10us(1);
    /* Write CTRL/STAT */
    Data = 0x50000000;
    SWD_TransferFunction(REQ_DP|REQ_W|REQ_ADDR_1, &Data, 7); co_delay_10us(1);
}

RAM_CODE void SWD_W_REG(uint32_t ADDR, uint32_t Value)
{
    uint32_t Data;
    /* Write Abort */
    Data = 0x0000001E;
    SWD_TransferFunction(REQ_DP|REQ_W|REQ_ADDR_0, &Data, 7);
    /* Write Select */
    Data = 0x00000000;
    SWD_TransferFunction(REQ_DP|REQ_W|REQ_ADDR_2, &Data, 0);
    /* Write CSW */
    Data = 0x23000012;
    SWD_TransferFunction(REQ_AP|REQ_W|REQ_ADDR_0, &Data, 0);
    /* Write TAR */
    Data = ADDR;
    SWD_TransferFunction(REQ_AP|REQ_W|REQ_ADDR_1, &Data, 0);
    /* Write DRW */
    Data = Value;
    SWD_TransferFunction(REQ_AP|REQ_W|REQ_ADDR_3, &Data, 7);
    /* Read RDBUFF */
    SWD_TransferFunction(REQ_DP|REQ_R|REQ_ADDR_3, &Data, 3);
}

RAM_CODE void SWD_R_REG(uint32_t ADDR, uint32_t *Buffer, uint32_t Length)
{
    int ERR;int first = 0;
    
    uint32_t Data;
    /* Write Abort */
    Data = 0x0000001E;
    SWD_TransferFunction(REQ_DP|REQ_W|REQ_ADDR_0, &Data, 7);
    /* Write Select */
    Data = 0x00000000;
    SWD_TransferFunction(REQ_DP|REQ_W|REQ_ADDR_2, &Data, 0);
    /* Write CSW */
    Data = 0x23000012;
    SWD_TransferFunction(REQ_AP|REQ_W|REQ_ADDR_0, &Data, 0);
    /* Write TAR */
    Data = ADDR;
    SWD_TransferFunction(REQ_AP|REQ_W|REQ_ADDR_1, &Data, 0);

    while(Length)
    {
        /* Read DRW */
        ERR = SWD_TransferFunction(REQ_AP|REQ_R|REQ_ADDR_3, Buffer, 3);
        
        if (ERR == 0)
        {
            if (first)
            {
                Length--;
                Buffer++;
            }
            first++;
        }
        else if (ERR == -2)
            Length = 0;
        SWD_DELAY();
        SWD_DELAY();
    }
}

RAM_CODE void SWD_W_SystemReg(uint32_t baudrate)
{
    SWD_IO_init();

    SWD_Connect();

    SWD_Enable_Debug();

    /* BIT24: enable cache clock */
    SWD_W_REG(0x50000008, 0x09000803);
    /* enable exchange memory clock */
    SWD_W_REG(0x50000010, 0x1000);
    
    ///config controller uart0 ref clk 48M
//    SWD_R_REG(0x50000000,&sys_reg,1);
//    SWD_W_REG(0x50000000,sys_reg|0x214);
    SWD_W_REG(0x50000000,0x20216);
    
    ///dll dlh access enable
//    SWD_R_REG(0x5003000c,&lcr,1);
//    SWD_W_REG(0x5003000c,lcr|0x80);
    SWD_W_REG(0x5003000c,0x83);
    
    if(baudrate == 921600){
        ///config uart0 921600 baudrate
        SWD_W_REG(0x50030000,3);
        SWD_W_REG(0x50030004,0);
        SWD_W_REG(0x500300c0,4);    
    }
    else if(baudrate == 3000000){
        ///config uart0 3M baudrate
        SWD_W_REG(0x50030000,1);
        SWD_W_REG(0x50030004,0);
        SWD_W_REG(0x500300c0,0);
    }

    ///dll dlh access disable
    SWD_W_REG(0x5003000c,0x03);
    
    ///1010 ioldo bypass
    SWD_W_REG(0x500f0000,0x00a83112);
    SWD_W_REG(0x500f0004,0x00000076);//pmuA8 = 0x76
    SWD_W_REG(0x500f0000,0x00a83113);
}
