#include <stddef.h>
#include <stdio.h>
#include "picowalker_rp2xxx_color_icons.h"

const color_icons_t icons_map[ICONS_COUNT] = {
    { 0x0460, 0x000000,  128,  8,  8 },  // 0x0460_pokeball.png
    { 0x0470, 0x000080,  128,  8,  8 },  // 0x0470_pokeball_event.png
    { 0x0488, 0x000100,  128,  8,  8 },  // 0x0488_item.png
    { 0x0498, 0x000180,  128,  8,  8 },  // 0x0498_item_event.png
    { 0x04A8, 0x000200,  128,  8,  8 },  // 0x04A8_map_icon.png
    { 0x04B8, 0x000280,  128,  8,  8 },  // 0x04B8_card_heart.png
    { 0x04C8, 0x000300,  128,  8,  8 },  // 0x04C8_card_spade.png
    { 0x04D8, 0x000380,  128,  8,  8 },  // 0x04D8_card_diamond.png
    { 0x04E8, 0x000400,  128,  8,  8 },  // 0x04E8_card_club.png
    { 0x0660, 0x000480,  128,  8,  8 },  // 0x0660_low_battery.png
    { 0x06D0, 0x000500,  768, 24, 16 },  // 0x06D0_talk_face_heart.png
    { 0x0730, 0x000800,  768, 24, 16 },  // 0x0730_talk_face_music.png
    { 0x0790, 0x000B00,  768, 24, 16 },  // 0x0790_talk_face_smile.png
    { 0x07F0, 0x000E00,  768, 24, 16 },  // 0x07F0_talk_face_neutral.png
    { 0x0850, 0x001100,  768, 24, 16 },  // 0x0850_talk_face_ellipsis.png
    { 0x08B0, 0x001400,  768, 24, 16 },  // 0x08B0_talk_exclamation.png
    { 0x1090, 0x001700,  512, 16, 16 },  // 0x1090_menu_icon_pokeradar.png
    { 0x10D0, 0x001900,  512, 16, 16 },  // 0x10D0_menu_icon_dowsing.png
    { 0x1110, 0x001B00,  512, 16, 16 },  // 0x1110_menu_icon_connect.png
    { 0x1150, 0x001D00,  512, 16, 16 },  // 0x1150_menu_icon_trainer_card.png
    { 0x1190, 0x001F00,  512, 16, 16 },  // 0x1190_menu_icon_inventory.png
    { 0x11D0, 0x002100,  512, 16, 16 },  // 0x11D0_menu_icon_settings.png
    { 0x1210, 0x002300,  512, 16, 16 },  // 0x1210_person_icon.png
    { 0x1390, 0x002500,  512, 16, 16 },  // 0x1390_route_small.png
    { 0x17D0, 0x002700,  768, 24, 16 },  // 0x17D0_speaker_off.png
    { 0x1830, 0x002A00,  768, 24, 16 },  // 0x1830_speaker_low.png
    { 0x1890, 0x002D00,  768, 24, 16 },  // 0x1890_speaker_high.png
    { 0x18F0, 0x003000,  256,  8, 16 },  // 0x18F0_contrast_demo.png
    { 0x1910, 0x003100, 1536, 32, 24 },  // 0x1910_treasure_large.png
    { 0x19D0, 0x003700, 1536, 32, 24 },  // 0x19D0_map_large.png
    { 0x1A90, 0x003D00, 1536, 32, 24 },  // 0x1A90_present_large.png
    { 0x1B50, 0x004300,  512, 16, 16 },  // 0x1B50_dowsing_bush_dark.png
    { 0x1B90, 0x004500,  512, 16, 16 },  // 0x1B90_dowsing_bush_light.png
    { 0x1CB0, 0x004700, 1536, 32, 24 },  // 0x1CB0_radar_bush.png
    { 0x1D70, 0x004D00,  512, 16, 16 },  // 0x1D70_radar_bubble_one.png
    { 0x1DB0, 0x004F00,  512, 16, 16 },  // 0x1DB0_radar_bubble_two.png
    { 0x1DF0, 0x005100,  512, 16, 16 },  // 0x1DF0_radar_bubble_three.png
    { 0x1E30, 0x005300,  512, 16, 16 },  // 0x1E30_radar_click.png
    { 0x1E70, 0x005500, 1024, 16, 32 },  // 0x1E70_radar_attack_hit.png
    { 0x1EF0, 0x005900, 1024, 16, 32 },  // 0x1EF0_radar_critical_hit.png
    { 0x1F70, 0x005D00, 1536, 32, 24 },  // 0x1F70_radar_appear_cloud.png
    { 0x2030, 0x006300,  128,  8,  8 },  // 0x2030_radar_hp_blip.png
    { 0x2040, 0x006380,  128,  8,  8 },  // 0x2040_radar_catch_effect.png
    { 0x2350, 0x006400, 2048, 32, 32 },  // 0x2350_pokewalker_big.png
    { 0x2450, 0x006C00,  256,  8, 16 },  // 0x2450_ir_arcs.png
    { 0x2470, 0x006D00,  128,  8,  8 },  // 0x2470_music_note.png
};


uint8_t* find_icon_by_eeprom_address(uint16_t eeprom_address) 
{
    int left = 0;
    int right = ICONS_COUNT - 1;

    while (left <= right) 
    {
        int mid = left + (right - left) / 2;
        uint16_t mid_addr = icons_map[mid].eeprom_address;

        if (mid_addr == eeprom_address) 
        {
            uint32_t offset = icons_map[mid].bin_offset;
            uint32_t size = icons_map[mid].size;

            if (offset + size > ICONS_BIN_SIZE) 
            {
                printf("[COLOR_ICON_ERROR] Address 0x%04X: bounds check failed (offset=0x%06X + size=%u > BIN_SIZE=%u)\n", eeprom_address, offset, size, ICONS_BIN_SIZE);
                return NULL; // Out of bounds
            }
            printf("[COLOR_ICON_FOUND] Address 0x%04X: offset=0x%06X, size=%u bytes, %ux%u pixels\n", eeprom_address, offset, size, icons_map[mid].width, icons_map[mid].height);
            return color_icons + offset;
        } 
        else if (mid_addr < eeprom_address) left = mid + 1;
        else right = mid - 1;
    }

    printf("[COLOR_ICON_MISS] Address 0x%04X: not found in lookup table\n", eeprom_address);
    return NULL;
}
