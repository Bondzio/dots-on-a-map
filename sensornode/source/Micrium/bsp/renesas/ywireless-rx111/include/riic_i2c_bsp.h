#ifndef _RIIC_I2C_BSP_H
#define _RIIC_I2C_BSP_H


void RIIC_BSP_I2C_init();
void  RIIC_BSP_I2C_SetCfg (long         freq,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir);
int RIIC_BSP_I2C_WrAndRd(int address, CPU_INT08U * data, int writeBytes, CPU_INT08U * read, int readBytes);
int RIIC_BSP_I2C_WrRd(int address, CPU_INT08U * data, CPU_INT08U * read, int bytes);

#endif
