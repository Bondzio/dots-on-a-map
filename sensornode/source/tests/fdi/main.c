#include <bsp_switch.h>

CPU_BOOLEAN my_switch_callback(CPU_INT08U sw, SWITCH_EVENT type) {
    return 0;
}


int main( void )
{
    BSP_SwitchIntReg(2, my_switch_callback);
    return 0;
}
