#include <stdbool.h>

#define DUI_IMPLEMENTATION
#include <DUI/DUI.h>

#include "debug.h"
#include "../common/log.h"
#include "render.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define SCREEN_SCALE 1

SDL_Window* dbg_window = NULL;
SDL_Renderer* dbg_renderer = NULL;
int dbg_window_id;

bool dbg_window_visible = false;

gbabus_t* bus;
arm7tdmi_t* cpu;
gba_ppu_t* ppu;

void debug_init(arm7tdmi_t* new_cpu, gba_ppu_t* new_ppu, gbabus_t* new_bus) {
    cpu = new_cpu;
    ppu = new_ppu;
    bus = new_bus;
}

void setup_dbg_sdl_window() {
    dbg_window = SDL_CreateWindow("dgb gba dbg",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  WINDOW_WIDTH * SCREEN_SCALE,
                                  WINDOW_HEIGHT * SCREEN_SCALE,
                                  SDL_WINDOW_SHOWN);


    dbg_window_id = SDL_GetWindowID(dbg_window);

    dbg_renderer = SDL_CreateRenderer(dbg_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_RenderSetScale(dbg_renderer, SCREEN_SCALE, SCREEN_SCALE);

    DUI_Init(dbg_window);

    DUI_Style * style = DUI_GetStyle();
    style->CharSize = 8 * SCREEN_SCALE;
    style->LineHeight = 10 * SCREEN_SCALE;

    if (dbg_renderer == NULL) {
        logfatal("SDL couldn't create a renderer! %s", SDL_GetError());
    }

    dbg_window_visible = true;
}

void teardown_dbg_sdl_window() {
    DUI_Term();
    SDL_DestroyRenderer(dbg_renderer);
    SDL_DestroyWindow(dbg_window);
    dbg_window_visible = false;
}

enum {
    TAB_CPU_REGISTERS,
    TAB_VIDEO_REGISTERS,
    TAB_TILE_DATA,
    TAB_TILE_MAP
};

int tab_index = TAB_VIDEO_REGISTERS;

#define cpsrflag(f, c) (f == 1?c:"-")
#define printcpsr(cpsr) DUI_Println("CPSR: %08Xh [%s%s%s%s%s%s%s]", cpsr.raw, cpsrflag(cpsr.N, "N"), cpsrflag(cpsr.Z, "Z"), \
         cpsrflag(cpsr.C, "C"), cpsrflag(cpsr.V, "V"), cpsrflag(cpsr.disable_irq, "I"), \
         cpsrflag(cpsr.disable_fiq, "F"), cpsrflag(cpsr.thumb, "T"))

#define print_bgcnt(n, reg) DUI_Println("BG%dCNT: %08Xh\n " \
            "priority: %d, chr_base: %d, mosiac: %d, 256color: %d, scr_base: %d, screen_size: %d\n\n" \
            , n, \
            reg.raw, \
            reg.priority, \
            reg.character_base_block, \
            reg.mosaic, \
            reg.is_256color, \
            reg.screen_base_block, \
            reg.screen_size)

#define print_bgofs(n, hofs, vofs) DUI_Println("BG%dOFS: %08Xh / %08Xh", n, hofs.raw, vofs.raw)

void dbg_tick() {
    if (dbg_window_visible) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            debug_handle_event(&event);
            gba_handle_event(&event);
        }
        DUI_Update();
        SDL_SetRenderDrawColor(dbg_renderer, 0x33, 0x33, 0x33, 0xFF);
        SDL_RenderClear(dbg_renderer);
        DUI_MoveCursor(8, 8);

        DUI_BeginTabBar();

        if (DUI_Tab("CPU Registers", TAB_CPU_REGISTERS, &tab_index)) {
            DUI_MoveCursor(8, 40);
            DUI_Panel(WINDOW_WIDTH - 16, WINDOW_HEIGHT - 48);
            DUI_Println("IF: %08Xh", bus->IF.raw);
            DUI_Println("Halt: %d\nIRQ: %d", cpu->halt, cpu->irq);
            DUI_Print("Mode: ");

            const char* execution_mode = cpu->cpsr.thumb ? "[THM]" : "[ARM]";

            switch (cpu->cpsr.mode) {
                case MODE_USER:
                    DUI_Println("User %s", execution_mode);
                    break;
                case MODE_FIQ:
                    DUI_Println("FIQ %s", execution_mode);
                    break;
                case MODE_SUPERVISOR:
                    DUI_Println("Supervisor %s", execution_mode);
                    break;
                case MODE_ABORT:
                    DUI_Println("Abort %s", execution_mode);
                    break;
                case MODE_IRQ:
                    DUI_Println("IRQ %s", execution_mode);
                    break;
                case MODE_UNDEFINED:
                    DUI_Println("Undefined %s", execution_mode);
                    break;
                case MODE_SYSTEM:
                    DUI_Println("System %s", execution_mode);
                    break;
            }

            printcpsr(cpu->cpsr);

            for (int r = 0; r < 16; r++) {
                DUI_Println("r%02d:  %08Xh", r, get_register(cpu, r));
            }
        }

        if (DUI_Tab("Video Registers", TAB_VIDEO_REGISTERS, &tab_index)) {
            DUI_MoveCursor(8, 40);
            DUI_Panel(WINDOW_WIDTH - 16, WINDOW_HEIGHT - 48);

            DUI_Println("DISPSTAT: 0x%08Xh\n vbl: %d\n hbl: %d\n vcnt %d\n IRQ:\n  vbl: %d\n  hbl: %d\n  vcount: %d\n  vcount_setting: %02Xh\n",
                        ppu->DISPSTAT.raw,
                        ppu->DISPSTAT.vblank,
                        ppu->DISPSTAT.hblank,
                        ppu->DISPSTAT.vcount,
                        ppu->DISPSTAT.vblank_irq_enable,
                        ppu->DISPSTAT.hblank_irq_enable,
                        ppu->DISPSTAT.vcount_irq_enable,
                        ppu->DISPSTAT.vcount_setting);

            DUI_Println("DISPCNT: %08Xh\n mode: %d\n cgbmode: %d\n display_frame_select: %d\n hblank_interval_free: %d\n obj_c_vrm: %s\n"
                        " forced_blank: %d\n display: bg0: %d bg1: %d bg2: %d bg3: %d win0: %d win1: %d objwin %d]\n\n",
                        ppu->DISPCNT.raw,
                        ppu->DISPCNT.mode,
                        ppu->DISPCNT.cgbmode,
                        ppu->DISPCNT.display_frame_select,
                        ppu->DISPCNT.hblank_interval_free,
                        ppu->DISPCNT.obj_character_vram_mapping ? "1D" : "2D",
                        ppu->DISPCNT.forced_blank,
                        ppu->DISPCNT.screen_display_bg0,
                        ppu->DISPCNT.screen_display_bg1,
                        ppu->DISPCNT.screen_display_bg2,
                        ppu->DISPCNT.screen_display_bg3,
                        ppu->DISPCNT.window0_display,
                        ppu->DISPCNT.window1_display,
                        ppu->DISPCNT.obj_window_display);

            print_bgcnt(0, ppu->BG0CNT);
            print_bgcnt(1, ppu->BG1CNT);
            print_bgcnt(2, ppu->BG2CNT);
            print_bgcnt(3, ppu->BG3CNT);

            print_bgofs(0, ppu->BG0HOFS, ppu->BG0VOFS);
            print_bgofs(1, ppu->BG1HOFS, ppu->BG1VOFS);
            print_bgofs(2, ppu->BG2HOFS, ppu->BG2VOFS);
            print_bgofs(3, ppu->BG3HOFS, ppu->BG3VOFS);
        }

        if (DUI_Tab("Tile Data", TAB_TILE_DATA, &tab_index)) {
            DUI_MoveCursor(8, 40);
            DUI_Panel(WINDOW_WIDTH - 16, WINDOW_HEIGHT - 48);
            if (DUI_Button("Dump VRAM")) {
                bool allzeroes = true;
                for (int x = 0; x < VRAM_SIZE; x += 0x10) {
                            half a = gba_read_half(0x06000000 + x);
                            half b = gba_read_half(0x06000000 + x + 0x2);
                            half c = gba_read_half(0x06000000 + x + 0x4);
                            half d = gba_read_half(0x06000000 + x + 0x6);
                            half e = gba_read_half(0x06000000 + x + 0x8);
                            half f = gba_read_half(0x06000000 + x + 0xA);
                            half g = gba_read_half(0x06000000 + x + 0xC);
                            half h = gba_read_half(0x06000000 + x + 0xE);
                            if (a != 0 || b != 0 || c != 0 || d != 0 || e != 0 || f != 0 || g != 0 || h != 0) {
                                allzeroes = false;
                            }
                    printf("%08X: %04X %04X %04X %04X %04X %04X %04X %04X\n",
                           0x06000000 + x, a, b, c, d, e, f, g, h);
                }
                if (allzeroes) {
                    printf("Warning: all zeroes!\n");
                }
            }
        }

        if (DUI_Tab("Tile Map", TAB_TILE_MAP, &tab_index)) {
            DUI_MoveCursor(8, 40);
            DUI_Panel(WINDOW_WIDTH - 16, WINDOW_HEIGHT - 48);
            DUI_Println("TAB #4");
        }

        SDL_RenderPresent(dbg_renderer);
    }
}

void set_dbg_window_visibility(bool visible) {
    if (visible && !dbg_window_visible) {
        setup_dbg_sdl_window();
    } else if (!visible & dbg_window_visible) {
        teardown_dbg_sdl_window();
    }
}

void handle_keydown(SDL_Keycode key) {
}

void handle_keyup(SDL_Keycode key) {
    if (key == SDLK_p || key == SDLK_ESCAPE) {
        set_dbg_window_visibility(false);
    }
}

void debug_handle_event(SDL_Event* event) {
    switch (event->type) {
        case SDL_KEYDOWN:
            if (event->key.windowID == dbg_window_id) {
                handle_keydown(event->key.keysym.sym);
            }
            break;
        case SDL_KEYUP:
            if (event->key.windowID == dbg_window_id) {
                handle_keyup(event->key.keysym.sym);
            }
            break;
        case SDL_MOUSEMOTION:
            if (event->key.windowID == dbg_window_id) {

            }
        default:
            break;
    }
}
