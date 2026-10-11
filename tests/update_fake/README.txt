Host-only fault injection for the actual UpdateDevice.h. cJSON.c/h: DaveGamble/cJSON v1.7.19, MIT license retained in both files. https://github.com/DaveGamble/cJSON/tree/v1.7.19
No runtime dependency or assets are added to the firmware. Network/SD/OTA/RTOS and crypto calls are faked; cryptographic library correctness and hardware power-loss behaviour require independent verification.
