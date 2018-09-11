#ifndef  BSP_UART_H_
#define  BSP_UART_H_


CPU_ISR SCI_BSP_UART_Rd_handler();
CPU_ISR SCI_BSP_UART_Err_handler();

void  SCI_BSP_UART_SetCfg (long         freq,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir);
void SCI_BSP_UART_init(char hwFlowControl, char prov);
int SCI_BSP_UART_WrRd(CPU_INT08U * data, void * ignored, int bytes);


#define SONAR_GROUP_READING_READY       0x00000001
#define PUBLISH_QUEUE_FULL              0x00000001
#define PUBLISH_QUEUE_NOT_FULL          0x00000002


#endif
