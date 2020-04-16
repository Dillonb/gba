#ifndef GBA_IOREG_NAMES_H
#define GBA_IOREG_NAMES_H
#define IO_DISPCNT      0x000
#define IO_DISPSTAT     0x004
#define IO_VCOUNT       0x006
#define IO_BG0CNT       0x008
#define IO_BG1CNT       0x00A
#define IO_BG2CNT       0x00C
#define IO_BG3CNT       0x00E
#define IO_BG0HOFS      0x010
#define IO_BG0VOFS      0x012
#define IO_BG1HOFS      0x014
#define IO_BG1VOFS      0x016
#define IO_BG2HOFS      0x018
#define IO_BG2VOFS      0x01A
#define IO_BG3HOFS      0x01C
#define IO_BG3VOFS      0x01E
#define IO_BG2PA        0x020
#define IO_BG2PB        0x022
#define IO_BG2PC        0x024
#define IO_BG2PD        0x026
#define IO_BG2X         0x028
#define IO_BG2Y         0x02C
#define IO_BG3PA        0x030
#define IO_BG3PB        0x032
#define IO_BG3PC        0x034
#define IO_BG3PD        0x036
#define IO_BG3X         0x038
#define IO_BG3Y         0x03C
#define IO_WIN0H        0x040
#define IO_WIN1H        0x042
#define IO_WIN0V        0x044
#define IO_WIN1V        0x046
#define IO_WININ        0x048
#define IO_WINOUT       0x04A
#define IO_MOSAIC       0x04C
#define IO_BLDCNT       0x050
#define IO_BLDALPHA     0x052
#define IO_BLDY         0x054
#define IO_SOUND1CNT_L  0x060
#define IO_SOUND1CNT_H  0x062
#define IO_SOUND1CNT_X  0x064
#define IO_SOUND2CNT_L  0x068
#define IO_SOUND2CNT_H  0x06C
#define IO_SOUND3CNT_L  0x070
#define IO_SOUND3CNT_H  0x072
#define IO_SOUND3CNT_X  0x074
#define IO_SOUND4CNT_L  0x078
#define IO_SOUND4CNT_H  0x07C
#define IO_SOUNDCNT_L   0x080
#define IO_SOUNDCNT_H   0x082
#define IO_SOUNDCNT_X   0x084
#define IO_SOUNDBIAS    0x088
#define WAVE_RAM0_L     0x090
#define WAVE_RAM0_H     0x092
#define WAVE_RAM1_L     0x094
#define WAVE_RAM1_H     0x096
#define WAVE_RAM2_L     0x098
#define WAVE_RAM2_H     0x09A
#define WAVE_RAM3_L     0x09C
#define WAVE_RAM3_H     0x09E
#define IO_FIFO_A       0x0A0
#define IO_FIFO_B       0x0A4
#define IO_DMA0SAD      0x0B0
#define IO_DMA0DAD      0x0B4
#define IO_DMA0CNT_L    0x0B8
#define IO_DMA0CNT_H    0x0BA
#define IO_DMA1SAD      0x0BC
#define IO_DMA1DAD      0x0C0
#define IO_DMA1CNT_L    0x0C4
#define IO_DMA1CNT_H    0x0C6
#define IO_DMA2SAD      0x0C8
#define IO_DMA2DAD      0x0CC
#define IO_DMA2CNT_L    0x0D0
#define IO_DMA2CNT_H    0x0D2
#define IO_DMA3SAD      0x0D4
#define IO_DMA3DAD      0x0D8
#define IO_DMA3CNT_L    0x0DC
#define IO_DMA3CNT_H    0x0DE
#define IO_TM0CNT_L     0x100
#define IO_TM0CNT_H     0x102
#define IO_TM1CNT_L     0x104
#define IO_TM1CNT_H     0x106
#define IO_TM2CNT_L     0x108
#define IO_TM2CNT_H     0x10A
#define IO_TM3CNT_L     0x10C
#define IO_TM3CNT_H     0x10E
#define IO_SIOMULTI0    0x120
#define IO_SIOMULTI1    0x122
#define IO_SIOMULTI2    0x124
#define IO_SIOMULTI3    0x126
#define IO_SIOCNT       0x128
#define IO_SIOMLT_SEND  0x12A
#define IO_KEYINPUT     0x130
#define IO_KEYCNT       0x132
#define IO_RCNT         0x134
#define IO_JOYCNT       0x140
#define IO_JOY_RECV     0x150
#define IO_JOY_TRANS    0x154
#define IO_JOYSTAT      0x158
#define IO_IE           0x200
#define IO_IF           0x202
#define IO_WAITCNT      0x204
#define IO_IME          0x208
#define IO_POSTFLG      0x300
#define IO_HALTCNT      0x301
#define IO_IMEM_CTRL    0x800

#define IO_UNDOCUMENTED_GREEN_SWAP 0x002

#endif //GBA_IOREG_NAMES_H
