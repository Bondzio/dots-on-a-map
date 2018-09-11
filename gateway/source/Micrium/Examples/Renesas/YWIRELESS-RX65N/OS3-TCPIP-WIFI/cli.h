#ifndef _CLI_H
#define _CLI_H


typedef struct st_config {
    int                 LoraID;
    int                 LoraMode;
    char                LoraKey[2];
    int                 LoraDestination;     
    int                 LoraPower;
    char                ssid[64];
    char                ssid_pw[32];
    char                api_key[49];
    char                proj_id[12];
    char                user_id[12];
    char                pw[32];
    char                registration_code[9];        
} config_t;

void cli(config_t * p_cfg);


#endif