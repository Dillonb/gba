#include "ppu.h"
#include "../common/log.h"
#include "../mem/gbabus.h"
#include "render.h"

gba_color_t bgbuf[4][GBA_SCREEN_X];
gba_color_t objbuf[GBA_SCREEN_X];
byte obj_priorities[GBA_SCREEN_X];

gba_ppu_t* init_ppu() {
    gba_ppu_t* ppu = malloc(sizeof(gba_ppu_t));

    ppu->DISPCNT.raw = 0;
    ppu->DISPSTAT.raw = 0;

    ppu->BG0CNT.raw = 0;
    ppu->BG1CNT.raw = 0;
    ppu->BG2CNT.raw = 0;
    ppu->BG3CNT.raw = 0;

    ppu->BG0HOFS.raw = 0;
    ppu->BG0VOFS.raw = 0;
    ppu->BG1HOFS.raw = 0;
    ppu->BG1VOFS.raw = 0;
    ppu->BG2HOFS.raw = 0;
    ppu->BG2VOFS.raw = 0;
    ppu->BG3HOFS.raw = 0;
    ppu->BG3VOFS.raw = 0;

    ppu->x = 0;
    ppu->y = 0;

    for (int i = 0; i < VRAM_SIZE; i++) {
        ppu->vram[i] = 0;
    }

    for (int i = 0; i < PRAM_SIZE; i++) {
        ppu->pram[i] = 0;
    }

    for (int i = 0; i < OAM_SIZE; i++) {
        ppu->oam[i] = 0;
    }

    return ppu;
}

bool is_hblank(gba_ppu_t* ppu) {
    return ppu->x > GBA_SCREEN_X;
}

bool is_vblank(gba_ppu_t* ppu) {
    return ppu->y > GBA_SCREEN_Y && ppu->y != 227;
}

#define PALETTE_BANK_BACKGROUND 0

#define FIVEBIT_TO_EIGHTBIT_COLOR(c) (c<<3)|(c&7)

void render_line_mode4(gba_ppu_t* ppu) {
    if (ppu->DISPCNT.screen_display_bg2) {
        for (int x = 0; x < GBA_SCREEN_X; x++) {
            int offset = x + (ppu->y * GBA_SCREEN_X); // Calculate this based on BG2X/Y/VOFS/HOFS/etc
            int index = ppu->DISPCNT.display_frame_select * 0xA000 + offset;
            int tile = ppu->vram[index];
            if (tile == 0) {
                ppu->screen[ppu->y][x].a = 0;
                ppu->screen[ppu->y][x].r = 0;
                ppu->screen[ppu->y][x].g = 0;
                ppu->screen[ppu->y][x].b = 0;
            } else {
                gba_color_t color;
                color.raw = gba_read_half(0x05000000 | (0x20 * PALETTE_BANK_BACKGROUND + 2 * tile)) & 0x7FFF;

                ppu->screen[ppu->y][x].a = 0xFF;
                ppu->screen[ppu->y][x].r = FIVEBIT_TO_EIGHTBIT_COLOR(color.r);
                ppu->screen[ppu->y][x].g = FIVEBIT_TO_EIGHTBIT_COLOR(color.g);
                ppu->screen[ppu->y][x].b = FIVEBIT_TO_EIGHTBIT_COLOR(color.b);
            }
        }
    } else {
        for (int x = 0; x < GBA_SCREEN_X; x++) {
            ppu->screen[ppu->y][x].a = 0;
            ppu->screen[ppu->y][x].r = 0;
            ppu->screen[ppu->y][x].g = 0;
            ppu->screen[ppu->y][x].b = 0;
        }
    }
}

typedef union obj_attr0 {
    struct {
        unsigned y:8;
        unsigned affine_object_mode:2;
        unsigned graphics_mode:2;
        bool mosaic:1;
        bool is_256color:1;
        unsigned shape:2;
    };
    half raw;
} obj_attr0_t;

typedef union obj_attr1 {
    struct {
        unsigned x:9;
        unsigned affine_index:5;
        unsigned size:2;
    };
    struct {
        unsigned:11;
        bool hflip:1;
        bool vflip:1;
        unsigned:3;
    };
    half raw;
} obj_attr1_t;

typedef union obj_attr2 {
    struct {
        unsigned tid:10;
        unsigned priority:2;
        unsigned pb:4;
    };
    half raw;
} obj_attr2_t;

// [shape][size]
int sprite_heights[3][4] = {
        {8,16,32,64},
        {16,32,32,64},
        {8,8,16,32}
};

// [shape][size]
int sprite_widths[3][4] = {
        {8,16,32,64},
        {8,8,16,32},
        {16,32,32,64}
};


#define OBJ_TILE_SIZE 0x20
void render_obj(gba_ppu_t* ppu) {
    obj_attr0_t attr0;
    obj_attr1_t attr1;
    obj_attr2_t attr2;
    for (int x = 0; x < GBA_SCREEN_X; x++) {
        obj_priorities[x] = 0;
        objbuf[x].transparent = true;
        objbuf[x].r = 0;
        objbuf[x].g = 0;
        objbuf[x].b = 0;
    }

    for (int sprite = 0; sprite < 128; sprite++) {
        attr0.raw = gba_read_half(0x07000000 + (sprite * 8) + 0);
        attr1.raw = gba_read_half(0x07000000 + (sprite * 8) + 2);
        attr2.raw = gba_read_half(0x07000000 + (sprite * 8) + 4);
        int in_tile_offset_divisor = attr0.is_256color ? 1 : 2;

        int height = sprite_heights[attr0.shape][attr1.size];
        //int tiles_high = height / 8;
        int width = sprite_widths[attr0.shape][attr1.size];
        int tiles_wide = width / 8;

        int sprite_y = ppu->y - attr0.y;
        if (attr1.vflip) {
            sprite_y = height - sprite_y - 1;
        }
        int sprite_tile_y = sprite_y / 8;

        if (ppu->y >= attr0.y && ppu->y <= attr0.y + height) { // If the sprite is visible, we should draw it.
            if (attr0.affine_object_mode == 0b00) { // Enabled
                unimplemented(attr0.is_256color, "256 color sprite");
                unimplemented(attr1.hflip, "hflip sprite")
                int tid = attr2.tid;
                int y_tid_offset;
                if (ppu->DISPCNT.obj_character_vram_mapping) { // 1D
                    y_tid_offset = tiles_wide * sprite_tile_y;
                } else { // 2D
                    y_tid_offset = 32 * sprite_tile_y;
                }
                tid += y_tid_offset;
                // At this point, we don't need to worry about 1D vs 2D
                // because in either case they'll be right next to each other in memory.
                for (int sprite_x = 0; sprite_x < width; sprite_x++) {
                    int screen_x = sprite_x + attr1.x;
                    int x_tid_offset = sprite_x / 8;
                    int tid_offset_by_x = tid + x_tid_offset;
                    word tile_address = 0x06010000 + tid_offset_by_x * OBJ_TILE_SIZE;

                    int tile_x = sprite_x % 8;
                    int tile_y = sprite_y % 8;

                    int in_tile_offset = tile_x + tile_y * 8;
                    tile_address += in_tile_offset / in_tile_offset_divisor;

                    byte tile = gba_read_byte(tile_address);

                    if (!attr0.is_256color) {
                        tile >>= (in_tile_offset % 2) * 4;
                        tile &= 0xF;
                    }

                    word palette_address = 0x05000200; // OBJ palette base
                    if (attr0.is_256color) {
                        palette_address += 2 * tile;
                    } else {
                        palette_address += (0x20 * attr2.pb + 2 * tile);
                    }
                    objbuf[screen_x].raw = gba_read_half(palette_address);
                    objbuf[screen_x].transparent = tile == 0; // This color should only be drawn if we need transparency
                }
            } else {
                unimplemented(attr0.affine_object_mode != 0b10, "Sprite with an affine object mode != 0b00 or 0b10")
            }
        }
    }
}

typedef union reg_se {
    half raw;
    struct {
        unsigned tid:10;
        bool hflip:1;
        bool vflip:1;
        unsigned pb:4;
    };
} reg_se_t;

#define SCREENBLOCK_SIZE 0x800
#define CHARBLOCK_SIZE  0x4000
void render_bg(gba_ppu_t* ppu, gba_color_t (*line)[GBA_SCREEN_X], BGCNT_t* bgcnt, int hofs, int vofs) {
    // Tileset (like pattern tables in the NES)
    word character_base_addr = 0x06000000 + bgcnt->character_base_block * CHARBLOCK_SIZE;
    // Tile map (like nametables in the NES)
    word screen_base_addr = 0x06000000 + bgcnt->screen_base_block * SCREENBLOCK_SIZE;

    int tile_size = bgcnt->is_256color ? 0x40 : 0x20;
    int in_tile_offset_divisor = bgcnt->is_256color ? 1 : 2;

    reg_se_t se;
    for (int x = 0; x < GBA_SCREEN_X; x++) {
        int screenblock_number;
        switch (bgcnt->screen_size) {
            case 0:
                // 0
                screenblock_number = 0;
                break;
            case 1:
                // 0 1
                screenblock_number = ((x + hofs) % 512) > 255 ? 1 : 0;
                break;
            case 2:
                // 0
                // 1
                screenblock_number = ((ppu->y + vofs) % 512) > 255 ? 1 : 0;
                break;
            case 3:
                // 0 1
                // 2 3
                screenblock_number =  ((x + hofs) % 512) > 255 ? 1 : 0;
                screenblock_number += ((ppu->y + vofs) % 512) > 255 ? 2 : 0;
                break;
            default:
                logfatal("Unimplemented screen size: %d", bgcnt->screen_size);

        }

        int tilemap_x = (x + hofs) % 256;
        int tilemap_y = (ppu->y + vofs) % 256;
        int se_number = (tilemap_x / 8) + (tilemap_y / 8) * 32;
        se.raw = gba_read_half(screen_base_addr + screenblock_number * SCREENBLOCK_SIZE + se_number * 2);

        // Find the tile
        word tile_address = character_base_addr + se.tid * tile_size;
        int tile_x = tilemap_x % 8;
        if (se.hflip) {
            tile_x = 7 - tile_x;
        }
        int tile_y = tilemap_y % 8;
        if (se.vflip) {
            tile_y = 7 - tile_y;
        }
        int in_tile_offset = tile_x + tile_y * 8;
        tile_address += in_tile_offset / in_tile_offset_divisor;

        byte tile = gba_read_byte(tile_address);

        if (!bgcnt->is_256color) {
            tile >>= (in_tile_offset % 2) * 4;
            tile &= 0xF;
        }

        word palette_address = 0x05000000;
        if (bgcnt->is_256color) {
            palette_address += 2 * tile;
        } else {
            palette_address += (0x20 * se.pb + 2 * tile);
        }
        (*line)[x].raw = gba_read_half(palette_address);
        (*line)[x].transparent = tile == 0; // This color should only be drawn if we need transparency
    }
}

int background_priorities[4];

void refresh_background_priorities(gba_ppu_t* ppu) {
    int insert_index = 0;
    // Insert all backgrounds with a certain priority, counting up
    for (int priority = 0; priority < 4; priority++) {
        if (ppu->BG0CNT.priority == priority) {
            background_priorities[insert_index] = 0;
            insert_index++;
        }
        if (ppu->BG1CNT.priority == priority) {
            background_priorities[insert_index] = 1;
            insert_index++;
        }
        if (ppu->BG2CNT.priority == priority) {
            background_priorities[insert_index] = 2;
            insert_index++;
        }
        if (ppu->BG3CNT.priority == priority) {
            background_priorities[insert_index] = 3;
            insert_index++;
        }
    }
}

void render_line_mode0(gba_ppu_t* ppu) {
    render_obj(ppu);
    if (ppu->DISPCNT.screen_display_bg0) {
        render_bg(ppu, &bgbuf[0], &ppu->BG0CNT, ppu->BG0HOFS.offset, ppu->BG0VOFS.offset);
    }

    if (ppu->DISPCNT.screen_display_bg1) {
        render_bg(ppu, &bgbuf[1], &ppu->BG1CNT, ppu->BG1HOFS.offset, ppu->BG1VOFS.offset);
    }

    if (ppu->DISPCNT.screen_display_bg2) {
        render_bg(ppu, &bgbuf[2], &ppu->BG2CNT, ppu->BG2HOFS.offset, ppu->BG2VOFS.offset);
    }

    if (ppu->DISPCNT.screen_display_bg3) {
        render_bg(ppu, &bgbuf[3], &ppu->BG3CNT, ppu->BG3HOFS.offset, ppu->BG3VOFS.offset);
    }

    refresh_background_priorities(ppu);

    bool bg_enabled[] = {
            ppu->DISPCNT.screen_display_bg0,
            ppu->DISPCNT.screen_display_bg1,
            ppu->DISPCNT.screen_display_bg2,
            ppu->DISPCNT.screen_display_bg3};

    for (int x = 0; x < GBA_SCREEN_X; x++) {
        bool non_transparent_drawn = false;
        for (int i = 3; i >= 0; i--) { // Draw them in reverse priority order, so the highest priority BG is drawn last.
            int bg = background_priorities[i];
            gba_color_t pixel = bgbuf[bg][x];
            bool should_draw = bg_enabled[bg];
            if (pixel.transparent) {
                // If the pixel is transparent, only draw it if we haven't drawn a non-transparent
                should_draw &= !non_transparent_drawn;
            }

            if (should_draw) {
                ppu->screen[ppu->y][x].a = 0xFF;
                ppu->screen[ppu->y][x].r = FIVEBIT_TO_EIGHTBIT_COLOR(pixel.r);
                ppu->screen[ppu->y][x].g = FIVEBIT_TO_EIGHTBIT_COLOR(pixel.g);
                ppu->screen[ppu->y][x].b = FIVEBIT_TO_EIGHTBIT_COLOR(pixel.b);

                if (!pixel.transparent) {
                    non_transparent_drawn = true;
                }
            }
        }
        // Draw OBJ layer
        // TODO: respect OBJ priorities

        if (!objbuf[x].transparent) {
            ppu->screen[ppu->y][x].a = 0xFF;
            ppu->screen[ppu->y][x].r = FIVEBIT_TO_EIGHTBIT_COLOR(objbuf[x].r);
            ppu->screen[ppu->y][x].g = FIVEBIT_TO_EIGHTBIT_COLOR(objbuf[x].g);
            ppu->screen[ppu->y][x].b = FIVEBIT_TO_EIGHTBIT_COLOR(objbuf[x].b);
        }
    }

}

void render_line(gba_ppu_t* ppu) {
    // Draw a pixel
    switch (ppu->DISPCNT.mode) {
        case 0:
            render_line_mode0(ppu);
            break;
        case 4:
            render_line_mode4(ppu);
            break;
        default:
            logfatal("Unknown graphics mode: %d", ppu->DISPCNT.mode)
    }
}

void ppu_step(gba_ppu_t* ppu) {
    // Update coords and set V/HBLANK flags
    ppu->x++;
    if (!ppu->DISPSTAT.hblank && is_hblank(ppu)) {
        if (ppu->DISPSTAT.hblank_irq_enable) {
            request_interrupt(IRQ_HBLANK);
        }
        ppu->DISPSTAT.hblank = true;
        if (ppu->y < GBA_SCREEN_Y && !ppu->DISPCNT.forced_blank) { // i.e. not VBlank
            render_line(ppu);
        }
    }
    if (ppu->x >= GBA_SCREEN_X + GBA_SCREEN_HBLANK) {
        ppu->x = 0;
        ppu->DISPSTAT.hblank = false;
        ppu->y++;

        if (ppu->y > GBA_SCREEN_Y + GBA_SCREEN_VBLANK) {
            ppu->y = 0;
            ppu->DISPSTAT.vblank = false;
        }

        if (!ppu->DISPSTAT.vblank && is_vblank(ppu)) {
            if (ppu->DISPSTAT.vblank_irq_enable) {
                request_interrupt(IRQ_VBLANK);
            }
            ppu->DISPSTAT.vblank = true;
            render_screen(&ppu->screen);
        }

        if (ppu->y == ppu->DISPSTAT.vcount_setting) {
            ppu->DISPSTAT.vcount = true;
            logwarn("VCOUNT flag is set!!!")
            if (ppu->DISPSTAT.vcount_irq_enable) {
                request_interrupt(IRQ_VCOUNT);
            }
        } else {
            ppu->DISPSTAT.vcount = false;
        }

    }
}
