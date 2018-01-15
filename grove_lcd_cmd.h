/*
 * grove_lcd_cmd.h
 * Copyright (C) 2017 Yuhei Horibe
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This is a command definitions for Grove Pi LCD display
 *
 */

// ******************************************
// LCD control command definitions
// ******************************************
#define CLEAR_DISPLAY              0x01
#define RETURN_HOME                0x02
#define ENTRY_MODE_SET             0x04
#define DISPLAY_ON_OFF_CTL         0x08
#define CURSOR_OR_DISPLAY_SHIFT    0x10
#define FUNCTION_SET               0x20
#define SET_CGRAM_ADDRESS          0x40
#define SET_DDRAM_ADDRESS          0x80

struct grove_lcd_cmd
{
    u8    command; // Command for Grove Pi LCD display
    union grove_lcd_cmd_params{
        // Entry mode set command parameter
        /*
         * shfit_entire_display
         * 0: Shift cursor only
         * 1: Scroll entire display
         *
         * id_flag
         * 0: Increment (Cursor will move to right)
         * 1: Decrement (Cursor will move to left)
         */
        union params_entry_mode_set{
            u8 all;
            struct params{
                u8 shift_entire_display : 1;
                u8 id_flag              : 1;
            } bits;
        } entry_mode_set;

        // Display on/off control parameter
        union params_display_on_off_ctl{
            u8 all;
            struct params{
                u8 cursor_blink_on_off  : 1;
                u8 cursor_on_off        : 1;
                u8 display_on_off       : 1;
            } bits:
        } display_on_off_ctl;

        // Cursor or display shift control parameter
        /*
         * rl_sel
         * 0: Shift cursor/display to left
         * 1: Shift cursor/display to right
         *
         * scroll_or_shift_sel
         * 0: Shift cursor only
         * 1: Shift entire display
         */
        union params_cursor_or_display_shift{
            u8 all;
            struct params{
                u8 reserved             : 2;
                u8 rl_sel               : 1;
                u8 scroll_or_shift_sel  : 1;
            } bits:
        } display_on_off_ctl;

        // Function set control parametr
        /*
         * format_mode
         * 0: Use 5x8  dot format display 
         * 1: Use 5x11 dot format display
         *
         * line_number
         * 0: Use 1-line display mode
         * 1: Use 2-line display mode
         *
         * data_length
         * 0: Use 4-bit data transfer
         * 1: Use 8-bit data transfer
         */
        union params_function_set{
            u8 all;
            struct params{
                u8 reserved             : 2;
                u8 format_mode          : 1;
                u8 line_number          : 1;
                u8 data_length          : 1;
            } bits;
        } function_set;

        // Set CGRAM address
        union params_set_cgram_addr{
            u8 all;
            struct params{
                u8 address              : 5;
            };
        } set_cgram_address;

        // Set DDRAM address
        union params_set_ddram_addr{
            u8 all;
            struct params{
                u8 address              : 6;
            };
        } set_ddram_address;
    } param;
};

