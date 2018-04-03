// Copyright (c) 2018 Jan Brittenson
// See LICENSE for details.

#ifndef _HSD04319W1_A_H_
#define _HSD04319W1_A_H_

namespace PanelInfo {

// Panel timing
enum {
    // Pixel clock
    PXCLOCK_MIN = 5000000,
    PXCLOCK_TYP = 9000000,  // 9MHz
    PXCLOCK_MAX = 12000000,

    // Vertical timing
    VERT_VISIBLE = 272,
    VERT_FRONT_PORCH = 8,
    VERT_BACK_PORCH = 8-1,  // Datasheet includes VSYNC in porch
    VSYNC_WIDTH = 1,
    VSYNC_MOVE = 0,

    // Horizontal timing
    HOR_VISIBLE = 480,
    HOR_FRONT_PORCH = 5,
    HOR_BACK_PORCH = 40-8,  // Datasheet includes HSYNC in porch
    HSYNC_WIDTH = 8,
    HSYNC_MOVE = 0,
    HSYNC_SUBPIXEL_POS = 0
};

};
#endif // _HSD04319W1_A_H_
