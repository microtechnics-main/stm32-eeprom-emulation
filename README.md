# STM32 EEPROM emulation

Proper way to emulate EEPROM memory based on using several pages of STM32 flash-memory. Handling includes:

- using two pages for possible errors elimination and maximum possible rewrite cycles increasing
- key-value system for data identification

<img src="https://microtechnics.ru/wp-content/uploads/2020/10/kopirovanie-dannyh-flash-pamyati-1-1536x521.jpg.webp" width="400">

Hardware for this project: STM32F103C8 and BluePill development board, but it was also tested on lots of different MCUs, so the migrating process is really simple. 

Software: IAR Embedded Workbench for ARM, HAL driver, STM32CubeMx - in general, standard tools set.

Full detailed description is as usual available on my [website](https://microtechnics.ru/emulyacziya-eeprom-na-baze-flash-pamyati-mikrokontrollerov-stm32/), I'll be glad to see you there :wave:
