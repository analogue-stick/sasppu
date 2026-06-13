#![allow(clippy::too_many_arguments)]
#![allow(clippy::cast_sign_loss)]
#![feature(portable_simd)]

/*!
 * The following is the original Rust version of SASPPU, of which the C/ASM
 * version is ported.
 */

use core::simd::prelude::*;
use std::rc::Rc;

use seq_macro::seq;

pub const WINDOW_A: u8 = 0b0001;
pub const WINDOW_B: u8 = 0b0010;
pub const WINDOW_AB: u8 = 0b0100;
pub const WINDOW_X: u8 = 0b1000;
pub const WINDOW_ALL: u8 = 0b1111;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Sprite {
    pub x:          i16,
    pub y:          i16,
    pub width:      u8,
    pub height:     u8,
    pub graphics_x: u8,
    pub graphics_y: u8,
    pub windows:    u8,
    pub flags:      u8,
}

impl Sprite {
    #[must_use]
    pub const fn new() -> Self {
        Sprite {
            x:          0,
            y:          0,
            width:      8,
            height:     8,
            graphics_x: 0,
            graphics_y: 0,
            windows:    0xFF,
            flags:      0,
        }
    }
}

impl Default for Sprite {
    fn default() -> Self {
        Self::new()
    }
}

pub const SPR_ENABLED: u8 = 1 << 0;
pub const SPR_FLIP_X: u8 = 1 << 1;
pub const SPR_FLIP_Y: u8 = 1 << 2;
pub const SPR_C_MATH: u8 = 1 << 3;
pub const SPR_DOUBLE: u8 = 1 << 4;
pub const SPR_USER_TYPE_SHIFT: u8 = 5;
pub const SPR_USER_TYPE: u8 = 7 << SPR_USER_TYPE_SHIFT;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct BackgroundState {
    pub scroll_x: i16,
    pub scroll_y: i16,
    pub windows:  u8,
    pub flags:    u8,
}

impl BackgroundState {
    #[must_use]
    pub const fn new() -> Self {
        BackgroundState {
            scroll_x: 0,
            scroll_y: 0,
            windows:  0xFF,
            flags:    0,
        }
    }
}

impl Default for BackgroundState {
    fn default() -> Self {
        Self::new()
    }
}

pub const BG_C_MATH: u8 = 1 << 0;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CMathState {
    pub screen_fade: u8,
    pub flags:       u8,
}

impl CMathState {
    #[must_use]
    pub const fn new() -> Self {
        CMathState {
            screen_fade: 0,
            flags:       0,
        }
    }
}

impl Default for CMathState {
    fn default() -> Self {
        Self::new()
    }
}

pub const CMATH_HALF_MAIN_SCREEN: u8 = 1 << 0;
pub const CMATH_DOUBLE_MAIN_SCREEN: u8 = 1 << 1;
pub const CMATH_HALF_SUB_SCREEN: u8 = 1 << 2;
pub const CMATH_DOUBLE_SUB_SCREEN: u8 = 1 << 3;
pub const CMATH_ADD_SUB_SCREEN: u8 = 1 << 4;
pub const CMATH_SUB_SUB_SCREEN: u8 = 1 << 5;
pub const CMATH_FADE_ENABLE: u8 = 1 << 6;
pub const CMATH_CMATH_ENABLE: u8 = 1 << 7;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MainState {
    pub mainscreen_colour: u16,
    pub subscreen_colour:  u16,

    // windowing
    pub window_1_left:  u8,
    pub window_1_right: u8,
    pub window_2_left:  u8,
    pub window_2_right: u8,
    pub bgcol_windows:  u8,

    pub flags: u8,
}

impl MainState {
    #[must_use]
    pub const fn new() -> Self {
        MainState {
            mainscreen_colour: 0,
            subscreen_colour:  0,
            // windowing
            window_1_left:     0,
            window_1_right:    255,
            window_2_left:     0,
            window_2_right:    255,
            bgcol_windows:     0xFF,
            flags:             0,
        }
    }
}

impl Default for MainState {
    fn default() -> Self {
        Self::new()
    }
}

pub const MAIN_CMATH_ENABLE: u8 = 1 << 0;
pub const MAIN_BGCOL_WINDOW_ENABLE: u8 = 1 << 1;

pub const BG_WIDTH_POWER: usize = 8;
pub const BG_HEIGHT_POWER: usize = 9;
pub const BG_WIDTH: usize = 1 << BG_WIDTH_POWER;
pub const BG_HEIGHT: usize = 1 << BG_HEIGHT_POWER;

pub const SPRITE_COUNT: usize = 256;
pub const SPRITE_CACHE: usize = 16;

pub type SpriteCache<'a> = [Option<&'a Sprite>; SPRITE_CACHE];

pub const SPR_WIDTH_POWER: usize = 8;
pub const SPR_HEIGHT_POWER: usize = 8;
pub const SPR_WIDTH: usize = 1 << SPR_WIDTH_POWER;
pub const SPR_HEIGHT: usize = 1 << SPR_HEIGHT_POWER;

pub const MAP_WIDTH_POWER: usize = 6;
pub const MAP_HEIGHT_POWER: usize = 6;
pub const MAP_WIDTH: usize = 1 << MAP_WIDTH_POWER;
pub const MAP_HEIGHT: usize = 1 << MAP_HEIGHT_POWER;

pub type BackgroundPlane = [u16x8; (BG_WIDTH / 8) * BG_HEIGHT];
pub type SpritePlane = [[u16x8; SPR_WIDTH / 8]; SPR_HEIGHT];
pub type SpriteState = [Sprite; SPRITE_COUNT];
pub type BackgroundMap = [[u16; MAP_WIDTH]; MAP_HEIGHT];

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HDMACommand {
    Noop,
    MainStateMainscreenColour,
    MainStateSubscreenColour,
    MainStateWindow1Left,
    MainStateWindow1Right,
    MainStateWindow2Left,
    MainStateWindow2Right,
    MainStateBgcolWindows,
    MainStateFlags,
    CMathStateScreenFade,
    CMathStateFlags,
    BackgroundX,
    BackgroundY,
    BackgroundWindows,
    BackgroundFlags,
}

impl HDMACommand {
    #[must_use]
    pub const fn new() -> Self {
        HDMACommand::Noop
    }
}

impl Default for HDMACommand {
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct HDMAEntry {
    pub command: HDMACommand,
    pub value:   u16,
}

impl HDMAEntry {
    #[must_use]
    pub const fn new() -> Self {
        HDMAEntry {
            command: HDMACommand::new(),
            value:   0,
        }
    }
}

impl Default for HDMAEntry {
    fn default() -> Self {
        Self::new()
    }
}

pub type HDMATable = [HDMAEntry; 240];

pub enum Command {
    BindMainState(MainState),
    BindCMathState(CMathState),
    BindBackgroundPlane(Rc<BackgroundPlane>),
    BindSpritePlane(Rc<SpritePlane>),
    BindBackgroundMap(Rc<BackgroundMap>),
    BindBackgroundState(BackgroundState),
    BindSpriteState(Rc<SpriteState>),
    ApplyHDMA(Rc<HDMATable>),
    BeginFrame,
    UpdateWindows,
    DrawBackground,
    DrawSprites(u8),
    EndFrame,
}

pub struct CommandBuffer(Vec<Command>);

impl CommandBuffer {
    #[must_use]
    pub fn compile(commands: Vec<Command>) -> Self {
        Self(commands)
    }
}

macro_rules! window_logic_window {
    (0, $window_1:expr, $window_2:expr) => {
        mask16x8::splat(false)
    };
    (1, $window_1:expr, $window_2:expr) => {
        ($window_1 & $window_2)
    };
}
macro_rules! window_macro {
    ($a:tt, $b:tt, $c:tt, $d:tt, $window_1:ident, $window_2:ident) => {
        window_logic_window!($a, $window_1, !$window_2)
            | window_logic_window!($b, !$window_1, $window_2)
            | window_logic_window!($c, $window_1, $window_2)
            | window_logic_window!($d, !$window_1, !$window_2)
    };
}

#[inline]
fn get_window(logic: u8, window_1: mask16x8, window_2: mask16x8) -> mask16x8 {
    match logic & 0xF {
        0x0 => window_macro!(0, 0, 0, 0, window_1, window_2),
        0x1 => window_macro!(1, 0, 0, 0, window_1, window_2),
        0x2 => window_macro!(0, 1, 0, 0, window_1, window_2),
        0x3 => window_macro!(1, 1, 0, 0, window_1, window_2),
        0x4 => window_macro!(0, 0, 1, 0, window_1, window_2),
        0x5 => window_macro!(1, 0, 1, 0, window_1, window_2),
        0x6 => window_macro!(0, 1, 1, 0, window_1, window_2),
        0x7 => window_macro!(1, 1, 1, 0, window_1, window_2),
        0x8 => window_macro!(0, 0, 0, 1, window_1, window_2),
        0x9 => window_macro!(1, 0, 0, 1, window_1, window_2),
        0xA => window_macro!(0, 1, 0, 1, window_1, window_2),
        0xB => window_macro!(1, 1, 0, 1, window_1, window_2),
        0xC => window_macro!(0, 0, 1, 1, window_1, window_2),
        0xD => window_macro!(1, 0, 1, 1, window_1, window_2),
        0xE => window_macro!(0, 1, 1, 1, window_1, window_2),
        0xF => window_macro!(1, 1, 1, 1, window_1, window_2),
        _ => unreachable!(),
    }
}

#[inline]
fn handle_windows<const LOGIC: u8>(
    window_1: &[mask16x8; 240 / 8],
    window_2: &[mask16x8; 240 / 8],
    main_col: &mut [u16x8; 240 / 8],
    sub_col: &mut [u16x8; 240 / 8],
    x: usize,
    col_in: u16x8,
) {
    let main_window =
        get_window(LOGIC & 0x0F, window_1[x], window_2[x]) & col_in.simd_ne(u16x8::splat(0));
    main_col[x] = main_window.select(col_in, main_col[x]);

    let sub_window =
        get_window(LOGIC >> 4, window_1[x], window_2[x]) & col_in.simd_ne(u16x8::splat(0));
    sub_col[x] = sub_window.select(col_in, sub_col[x]);
}

type HandleWindowType = fn(
    &[mask16x8; 240 / 8],
    &[mask16x8; 240 / 8],
    &mut [u16x8; 240 / 8],
    &mut [u16x8; 240 / 8],
    usize,
    u16x8,
);

seq!(N in 0..256 {
static HANDLE_WINDOW_LOOKUP: [HandleWindowType; 256] =
    [
        #(
            handle_windows::<N>,
        )*
    ];
});

#[inline]
fn select_correct_handle_window(windows: u8) -> HandleWindowType {
    HANDLE_WINDOW_LOOKUP[windows as usize]
}

#[inline]
fn swimzleoo(a: u16x8, b: u16x8, offset: usize) -> u16x8 {
    match offset {
        0 => a,
        1 => simd_swizzle!(a, b, [1, 2, 3, 4, 5, 6, 7, 8]),
        2 => simd_swizzle!(a, b, [2, 3, 4, 5, 6, 7, 8, 9]),
        3 => simd_swizzle!(a, b, [3, 4, 5, 6, 7, 8, 9, 10]),
        4 => simd_swizzle!(a, b, [4, 5, 6, 7, 8, 9, 10, 11]),
        5 => simd_swizzle!(a, b, [5, 6, 7, 8, 9, 10, 11, 12]),
        6 => simd_swizzle!(a, b, [6, 7, 8, 9, 10, 11, 12, 13]),
        7 => simd_swizzle!(a, b, [7, 8, 9, 10, 11, 12, 13, 14]),
        _ => unreachable!(),
    }
}

#[inline]
fn handle_bg(
    state: BackgroundState, // a9
    map: &BackgroundMap,    // a10
    graphics: &BackgroundPlane,
    window_handler: HandleWindowType,
    main_col: &mut [u16x8; 240 / 8], // q0
    sub_col: &mut [u16x8; 240 / 8],  // q1
    y: i16,                          // a3
    window_1: &[mask16x8; 240 / 8],  // q2
    window_2: &[mask16x8; 240 / 8],  // q3
) {
    let y_pos = (((y + state.scroll_y) as usize) >> 3) & ((MAP_HEIGHT) - 1);
    let mut x_pos = (((240 - 8 + state.scroll_x) as usize) >> 3) & ((MAP_WIDTH) - 1);
    let offset_x = ((state.scroll_x) & 0x7u16.cast_signed()) as usize;
    let offset_y = ((y + state.scroll_y) & 0x7u16.cast_signed()) as usize;

    let bg_map = map[y_pos][x_pos]; // -> q5
    let mut bg_1 = if (bg_map & 0b10) > 0 {
        graphics[(bg_map >> 2) as usize + ((7 - offset_y) * (BG_WIDTH >> 3))]
    } else {
        graphics[(bg_map >> 2) as usize + (offset_y * (BG_WIDTH >> 3))]
    }; // -> q5

    if (bg_map & 0b01) > 0 {
        bg_1 = bg_1.reverse();
    }

    let mut bg_2; // -> q4

    for x in (0..(240 / 8)).rev() {
        bg_2 = bg_1;
        x_pos = (x_pos.wrapping_sub(1)) & ((MAP_WIDTH) - 1);

        let bg_map = map[y_pos][x_pos]; // -> q5
        bg_1 = if (bg_map & 0b10) > 0 {
            graphics[(bg_map >> 2) as usize + ((7 - offset_y) * (BG_WIDTH >> 3))]
        } else {
            graphics[(bg_map >> 2) as usize + (offset_y * (BG_WIDTH >> 3))]
        }; // -> q5

        if (bg_map & 0b01) > 0 {
            bg_1 = bg_1.reverse();
        }

        let mut bg = swimzleoo(bg_1, bg_2, offset_x); // q4, q5 -> q4

        if state.flags & BG_C_MATH > 0 {
            bg |= u16x8::splat(0x8000); // q5; q4, q5 -> q4
        }

        window_handler(window_1, window_2, main_col, sub_col, x, bg);
    }
}

#[inline]
fn handle_sprite<const FLIP_X: bool, const FLIP_Y: bool, const CMATH: bool, const DOUBLE: bool>(
    sprite: &Sprite,
    graphics: &SpritePlane,
    main_col: &mut [u16x8; 240 / 8], // q0
    sub_col: &mut [u16x8; 240 / 8],  // q1
    y: i16,
    window_1: &[mask16x8; 240 / 8], // q2
    window_2: &[mask16x8; 240 / 8], // q3
) {
    assert_eq!(sprite.width & 0x7, 0);
    assert!(sprite.width > 0);

    let sprite_width = if DOUBLE {
        sprite.width << 1
    } else {
        sprite.width
    };

    let offset = (8 - (sprite.x & 0x7i16)) as usize & 0x07;

    let mut offset_y = y - sprite.y;
    if FLIP_Y {
        offset_y = i16::from(sprite_width) - offset_y - 1;
    }
    if DOUBLE {
        offset_y >>= 1;
    }
    let offset_y = offset_y as usize;

    let mut x_pos = if FLIP_X { -8 } else { sprite.width as isize };

    let mut spr_1 = u16x8::splat(0);
    let mut spr_2;

    let mut start_x = (sprite.x as isize) / 8;
    let mut end_x = ((sprite.x + i16::from(sprite_width)) as isize) / 8;

    if sprite.x.trailing_zeros() >= 3 {
        start_x -= 1;
        end_x -= 1;
    }

    let start_x = start_x;

    let mut x = end_x;
    while x >= start_x {
        x_pos = if FLIP_X { x_pos + 8 } else { x_pos - 8 };

        spr_2 = spr_1;

        spr_1 = if (FLIP_X && x_pos >= sprite.width as isize) || (!FLIP_X && x_pos < 0) {
            // q5
            u16x8::splat(0)
        } else {
            graphics[offset_y + sprite.graphics_y as usize]
                [(x_pos.cast_unsigned() >> 3) + sprite.graphics_x as usize]
        };

        if DOUBLE {
            let mut spr_1_high;
            (spr_1, spr_1_high) = spr_1.interleave(spr_1);

            if FLIP_X {
                (spr_1_high, spr_1) = (spr_1.reverse(), spr_1_high.reverse());
            }

            if (0..30).contains(&x) {
                let mut spr_col = swimzleoo(spr_1_high, spr_2, offset); // q4

                if CMATH {
                    spr_col |= u16x8::splat(0x8000);
                }

                select_correct_handle_window(sprite.windows)(
                    window_1,
                    window_2,
                    main_col,
                    sub_col,
                    x.cast_unsigned(),
                    spr_col,
                );
            }
            x -= 1;

            if (0..30).contains(&x) {
                let mut spr_col = swimzleoo(spr_1, spr_1_high, offset); // q4

                if CMATH {
                    spr_col |= u16x8::splat(0x8000);
                }

                select_correct_handle_window(sprite.windows)(
                    window_1,
                    window_2,
                    main_col,
                    sub_col,
                    x.cast_unsigned(),
                    spr_col,
                );
            }
            x -= 1;
        } else {
            if FLIP_X {
                spr_1 = spr_1.reverse();
            }

            if (0..30).contains(&x) {
                let mut spr_col = swimzleoo(spr_1, spr_2, offset); // q4

                if CMATH {
                    spr_col |= u16x8::splat(0x8000);
                }

                select_correct_handle_window(sprite.windows)(
                    window_1,
                    window_2,
                    main_col,
                    sub_col,
                    x.cast_unsigned(),
                    spr_col,
                );
            }
            x -= 1;
        }
    }
}

macro_rules! generate_handle_sprites {
    ($consts:expr) => {
        handle_sprite::<
            { (((($consts) as u16) & 0b00000001) > 0) },
            { (((($consts) as u16) & 0b00000010) > 0) },
            { (((($consts) as u16) & 0b00000100) > 0) },
            { (((($consts) as u16) & 0b00001000) > 0) },
        >
    };
}

type HandleSpriteType = fn(
    &Sprite,
    &SpritePlane,
    &mut [u16x8; 240 / 8],
    &mut [u16x8; 240 / 8],
    i16,
    &[mask16x8; 240 / 8],
    &[mask16x8; 240 / 8],
);

seq!(N in 0..16 {
static HANDLE_SPRITE_LOOKUP: [HandleSpriteType; 16] =
    [
        #(
        generate_handle_sprites!(N),
        )*
    ];
});

#[inline]
fn select_correct_handle_sprite(state: &Sprite) -> HandleSpriteType {
    HANDLE_SPRITE_LOOKUP[(state.flags >> 2) as usize]
}

macro_rules! split_main {
    ($main_col:ident, $mask:ident) => {{
        let main_r: i16x8 = (($main_col << 0) & $mask).cast();
        let main_g: i16x8 = (($main_col << 5) & $mask).cast();
        let main_b: i16x8 = (($main_col << 10) & $mask).cast();
        (main_r, main_g, main_b)
    }};
}

macro_rules! double_screen {
    ($main_r:ident, $main_g:ident, $main_b:ident) => {
        $main_r = $main_r.saturating_add($main_r);
        $main_g = $main_r.saturating_add($main_g);
        $main_b = $main_r.saturating_add($main_b);
    };
}

macro_rules! halve_screen {
    ($main_r:ident, $main_g:ident, $main_b:ident) => {
        $main_r >>= 1;
        $main_g >>= 1;
        $main_b >>= 1;
    };
}

macro_rules! add_screens {
    ($main_r:ident, $main_g:ident, $main_b:ident, $sub_r:ident, $sub_g:ident, $sub_b:ident) => {
        $main_r = $main_r.saturating_add($sub_r);
        $main_g = $main_g.saturating_add($sub_g);
        $main_b = $main_b.saturating_add($sub_b);
    };
}

macro_rules! sub_screens {
    ($main_r:ident, $main_g:ident, $main_b:ident, $sub_r:ident, $sub_g:ident, $sub_b:ident) => {
        $main_r -= $sub_r;
        $main_g -= $sub_g;
        $main_b -= $sub_b;
        $main_r = $main_r
            .simd_lt(i16x8::splat(0))
            .select(i16x8::splat(0), $main_r); // maps to EE.VRELU.S16 x, 0, 0
        $main_g = $main_g
            .simd_lt(i16x8::splat(0))
            .select(i16x8::splat(0), $main_g);
        $main_b = $main_b
            .simd_lt(i16x8::splat(0))
            .select(i16x8::splat(0), $main_b);
    };
}

#[inline]
fn no_cmath_shift(main_col: &mut [u16x8; 240 / 8]) {
    for x in (0..(240 / 8)).rev() {
        main_col[x] = ((main_col[x] & u16x8::splat(0b0111_1111_1110_0000)) << 1)
            | (main_col[x] & u16x8::splat(0b0001_1111));
    }
}

#[allow(clippy::similar_names)]
#[inline]
fn handle_cmath<
    const HALF_MAIN_SCREEN: bool,
    const DOUBLE_MAIN_SCREEN: bool,
    const HALF_SUB_SCREEN: bool,
    const DOUBLE_SUB_SCREEN: bool,
    const ADD_SUB_SCREEN: bool,
    const SUB_SUB_SCREEN: bool,
    const FADE_ENABLE: bool,
    const CMATH_ENABLE: bool,
>(
    cmath_state: CMathState,
    main_col: &mut [u16x8; 240 / 8], // q0
    sub_col: &mut [u16x8; 240 / 8],  // q1
) {
    if FADE_ENABLE || CMATH_ENABLE {
        let mask = u16x8::splat(0b0111_1100_0000_0000);
        for x in (0..(240 / 8)).rev() {
            let this_main_col = main_col[x];
            let use_cmath = this_main_col.simd_ge(u16x8::splat(0x8000));
            let (mut main_r, mut main_g, mut main_b) = split_main!(this_main_col, mask);
            if CMATH_ENABLE {
                let this_sub_col = sub_col[x];
                let use_cmath = use_cmath & this_sub_col.simd_ne(u16x8::splat(0x0000));
                let (mut sub_r, mut sub_g, mut sub_b) = split_main!(this_sub_col, mask);

                let main_r_bak = main_r;

                let main_g_bak = main_g;
                let main_b_bak = main_b;

                if DOUBLE_MAIN_SCREEN {
                    double_screen!(main_r, main_g, main_b);
                }
                if HALF_MAIN_SCREEN {
                    halve_screen!(main_r, main_g, main_b);
                }
                if DOUBLE_SUB_SCREEN {
                    double_screen!(sub_r, sub_g, sub_b);
                }
                if HALF_SUB_SCREEN {
                    halve_screen!(sub_r, sub_g, sub_b);
                }
                if ADD_SUB_SCREEN {
                    add_screens!(main_r, main_g, main_b, sub_r, sub_g, sub_b);
                }
                if SUB_SUB_SCREEN {
                    sub_screens!(main_r, main_g, main_b, sub_r, sub_g, sub_b);
                }

                main_r = use_cmath.select(main_r, main_r_bak);
                main_g = use_cmath.select(main_g, main_g_bak);
                main_b = use_cmath.select(main_b, main_b_bak);
            }
            if FADE_ENABLE {
                let fade = i16x8::splat(i16::from(cmath_state.screen_fade));
                main_r = (main_r >> 8) * fade;
                main_g = (main_g >> 8) * fade;
                main_b = (main_b >> 8) * fade;
            }
            let mut main_r: u16x8 = main_r.cast();
            let mut main_g: u16x8 = main_g.cast();
            let mut main_b: u16x8 = main_b.cast();
            main_r &= mask;
            main_g &= mask;
            main_b &= mask;
            main_col[x] = (main_r << 1) | (main_g >> 4) | (main_b >> 10);
        }
    } else {
        no_cmath_shift(main_col);
    }
}

macro_rules! generate_handle_cmaths {
    ($consts:expr) => {
        handle_cmath::<
            { ($consts) & CMATH_HALF_MAIN_SCREEN > 0 },
            { ($consts) & CMATH_DOUBLE_MAIN_SCREEN > 0 },
            { ($consts) & CMATH_HALF_SUB_SCREEN > 0 },
            { ($consts) & CMATH_DOUBLE_SUB_SCREEN > 0 },
            { ($consts) & CMATH_ADD_SUB_SCREEN > 0 },
            { ($consts) & CMATH_SUB_SUB_SCREEN > 0 },
            { ($consts) & CMATH_FADE_ENABLE > 0 },
            { ($consts) & CMATH_CMATH_ENABLE > 0 },
        >
    };
}

type HandleCMathType = fn(CMathState, &mut [u16x8; 240 / 8], &mut [u16x8; 240 / 8]);

seq!(N in 0..256 {
static HANDLE_CMATH_LOOKUP: [HandleCMathType; 256] =
    [
        #(
        generate_handle_cmaths!(N),
        )*
    ];
});

#[inline]
fn select_correct_handle_cmaths(state: CMathState) -> HandleCMathType {
    HANDLE_CMATH_LOOKUP[state.flags as usize]
}

fn command_update_windows(
    main_state: Option<&MainState>,
    window_1_cache: &mut [mask16x8; 240 / 8],
    window_2_cache: &mut [mask16x8; 240 / 8],
) {
    *window_1_cache = [mask16x8::default(); 240 / 8];
    *window_2_cache = [mask16x8::default(); 240 / 8];

    if let Some(main_state) = main_state {
        // x_window = q3
        let mut x_window = u16x8::from_array([0, 1, 2, 3, 4, 5, 6, 7]) + u16x8::splat(240 - 8);
        for x in (0..(240 / 8)).rev() {
            // window_1 = q2
            window_1_cache[x] = (x_window
                .simd_gt(u16x8::splat(u16::from(main_state.window_1_left)))
                | x_window.simd_eq(u16x8::splat(u16::from(main_state.window_1_left))))
                & (x_window.simd_lt(u16x8::splat(u16::from(main_state.window_1_right)))
                    | x_window.simd_eq(u16x8::splat(u16::from(main_state.window_1_right))));
            // window_2 = q3
            window_2_cache[x] = (x_window
                .simd_gt(u16x8::splat(u16::from(main_state.window_2_left)))
                | x_window.simd_eq(u16x8::splat(u16::from(main_state.window_2_left))))
                & (x_window.simd_lt(u16x8::splat(u16::from(main_state.window_2_right)))
                    | x_window.simd_eq(u16x8::splat(u16::from(main_state.window_2_right))));

            x_window -= u16x8::splat(8);
        }
    }
}

fn command_begin_frame(
    main_state: Option<&MainState>,
    main_screen: &mut [u16x8; 240 / 8],
    sub_screen: &mut [u16x8; 240 / 8],
    window_1_cache: &[mask16x8; 240 / 8],
    window_2_cache: &[mask16x8; 240 / 8],
) {
    if let Some(main_state) = main_state {
        if main_state.flags & MAIN_BGCOL_WINDOW_ENABLE > 0 {
            for x in (0..(240 / 8)).rev() {
                main_screen[x] = u16x8::splat(0);
                sub_screen[x] = u16x8::splat(0);
                select_correct_handle_window(main_state.bgcol_windows & 0xF0)(
                    window_1_cache,
                    window_2_cache,
                    main_screen,
                    sub_screen,
                    x,
                    u16x8::splat(main_state.subscreen_colour),
                );
                select_correct_handle_window(main_state.bgcol_windows & 0x0F)(
                    window_1_cache,
                    window_2_cache,
                    main_screen,
                    sub_screen,
                    x,
                    u16x8::splat(main_state.mainscreen_colour),
                );
            }
        } else {
            for x in (0..(240 / 8)).rev() {
                main_screen[x] = u16x8::splat(main_state.mainscreen_colour);
                sub_screen[x] = u16x8::splat(main_state.subscreen_colour);
            }
        }
    }
}

fn command_draw_background(
    background_state: Option<&BackgroundState>,
    background_map: Option<&Rc<BackgroundMap>>,
    background_plane: Option<&Rc<BackgroundPlane>>,
    main_screen: &mut [u16x8; 240 / 8],
    sub_screen: &mut [u16x8; 240 / 8],
    window_1_cache: &[mask16x8; 240 / 8],
    window_2_cache: &[mask16x8; 240 / 8],
    y: u8,
) {
    if let Some(background_state) = background_state
        && let Some(background_map) = background_map
        && let Some(background_plane) = background_plane
    {
        handle_bg(
            *background_state,
            background_map,
            background_plane,
            select_correct_handle_window(background_state.windows),
            main_screen,
            sub_screen,
            i16::from(y),
            window_1_cache,
            window_2_cache,
        );
    }
}

fn command_draw_sprites(
    sprite_plane: Option<&Rc<SpritePlane>>,
    sprite_cache: &SpriteCache,
    main_screen: &mut [u16x8; 240 / 8],
    sub_screen: &mut [u16x8; 240 / 8],
    window_1_cache: &[mask16x8; 240 / 8],
    window_2_cache: &[mask16x8; 240 / 8],
    y: u8,
) {
    if let Some(sprite_plane) = sprite_plane {
        for spr in sprite_cache.iter().rev() {
            if spr.is_none() {
                continue;
            }
            select_correct_handle_sprite(spr.unwrap())(
                spr.unwrap(),
                sprite_plane,
                main_screen,
                sub_screen,
                i16::from(y),
                window_1_cache,
                window_2_cache,
            );
        }
    }
}

fn command_end_frame(
    main_state: Option<&MainState>,
    cmath_state: Option<&CMathState>,
    main_screen: &mut [u16x8; 240 / 8],
    sub_screen: &mut [u16x8; 240 / 8],
) {
    if let Some(main_state) = main_state
        && let Some(cmath_state) = cmath_state
    {
        if main_state.flags & MAIN_CMATH_ENABLE > 0 {
            select_correct_handle_cmaths(*cmath_state)(*cmath_state, main_screen, sub_screen);
        } else {
            no_cmath_shift(main_screen);
        }
    }
}

#[allow(clippy::cast_possible_truncation)]
fn command_apply_hdma(
    main_state: Option<&mut MainState>,
    cmath_state: Option<&mut CMathState>,
    background_state: Option<&mut BackgroundState>,
    table: &HDMATable,
    y: u8,
) {
    let entry = table[y as usize];

    match entry.command {
        HDMACommand::MainStateMainscreenColour => {
            if let Some(main_state) = main_state {
                main_state.mainscreen_colour = entry.value;
            }
        },
        HDMACommand::MainStateSubscreenColour => {
            if let Some(main_state) = main_state {
                main_state.subscreen_colour = entry.value;
            }
        },
        HDMACommand::MainStateWindow1Left => {
            if let Some(main_state) = main_state {
                main_state.window_1_left = entry.value as u8;
            }
        },
        HDMACommand::MainStateWindow1Right => {
            if let Some(main_state) = main_state {
                main_state.window_1_right = entry.value as u8;
            }
        },
        HDMACommand::MainStateWindow2Left => {
            if let Some(main_state) = main_state {
                main_state.window_2_left = entry.value as u8;
            }
        },
        HDMACommand::MainStateWindow2Right => {
            if let Some(main_state) = main_state {
                main_state.window_2_right = entry.value as u8;
            }
        },
        HDMACommand::MainStateBgcolWindows => {
            if let Some(main_state) = main_state {
                main_state.bgcol_windows = entry.value as u8;
            }
        },
        HDMACommand::MainStateFlags => {
            if let Some(main_state) = main_state {
                main_state.flags = entry.value as u8;
            }
        },
        HDMACommand::CMathStateScreenFade => {
            if let Some(cmath_state) = cmath_state {
                cmath_state.screen_fade = entry.value as u8;
            }
        },
        HDMACommand::CMathStateFlags => {
            if let Some(cmath_state) = cmath_state {
                cmath_state.flags = entry.value as u8;
            }
        },
        HDMACommand::BackgroundX => {
            if let Some(background_state) = background_state {
                background_state.scroll_x = entry.value.cast_signed();
            }
        },
        HDMACommand::BackgroundY => {
            if let Some(background_state) = background_state {
                background_state.scroll_y = entry.value.cast_signed();
            }
        },
        HDMACommand::BackgroundWindows => {
            if let Some(background_state) = background_state {
                background_state.windows = entry.value as u8;
            }
        },
        HDMACommand::BackgroundFlags => {
            if let Some(background_state) = background_state {
                background_state.flags = entry.value as u8;
            }
        },
        HDMACommand::Noop => {},
    }
}

fn handle_sprite_cache<'a, 'b>(
    sprite_state: Option<&'a Rc<SpriteState>>,
    y: u8,
    sprite_cache: &mut SpriteCache<'b>,
    user_type: u8,
) where
    'a: 'b,
{
    let mut sprites_index = 0;
    if let Some(sprite_state) = sprite_state {
        for spr in sprite_state.iter() {
            let flags = spr.flags;
            let windows = spr.windows;
            let iy = i16::from(y);

            let spr_height = i16::from(spr.height);
            let spr_width = i16::from(spr.width);

            let main_screen_enable = (windows & 0x0F) > 0;
            let sub_screen_enable = (windows & 0xF0) > 0;

            let enabled = (flags & SPR_ENABLED) > 0;
            // let flip_x = (flags & SPR_FLIP_X) > 0;
            // let flip_y = (flags & SPR_FLIP_Y) > 0;
            // let cmath_enabled = (flags & SPR_C_MATH) > 0;
            let double_enabled = (flags & SPR_DOUBLE) > 0;
            let sprite_user_type = (flags & SPR_USER_TYPE) >> SPR_USER_TYPE_SHIFT;

            // If not enabled, skip
            if !enabled {
                continue;
            }

            // If user type does not match, skip
            if sprite_user_type != user_type {
                continue;
            }

            // If we've hit the limit, skip
            if sprites_index == SPRITE_CACHE {
                continue;
            }

            let window_enabled = (main_screen_enable) || (sub_screen_enable);
            let top_border = spr.y <= iy;
            let bottom_border = if double_enabled {
                spr.y > (iy - (spr_height << 1))
            } else {
                spr.y > (iy - (spr_height))
            };
            let right_border = spr.x < 240;
            let left_border = if double_enabled {
                spr.x > -(spr_width << 1)
            } else {
                spr.x > -(spr_width)
            };

            if window_enabled && top_border && bottom_border && right_border && left_border {
                sprite_cache[sprites_index] = Some(spr);
                sprites_index += 1;

                if sprites_index == SPRITE_CACHE {
                    break;
                }
            }
        }
    }
    while sprites_index < SPRITE_CACHE {
        sprite_cache[sprites_index] = None;
        sprites_index += 1;
    }
}

#[allow(clippy::too_many_lines)]
pub fn render(screen: &mut [[u16; 240]; 240], command_buffer: &CommandBuffer) {
    let mut main_screen = [u16x8::splat(0); 240 / 8];
    let mut sub_screen = [u16x8::splat(0); 240 / 8];
    let mut sprite_cache: SpriteCache;
    let mut window_1_cache: [mask16x8; 240 / 8];
    let mut window_2_cache: [mask16x8; 240 / 8];
    let mut this_main_state: Option<MainState> = None;
    let mut this_cmath_state: Option<CMathState> = None;
    let mut this_background_plane: Option<Rc<BackgroundPlane>> = None;
    let mut this_sprite_plane: Option<Rc<SpritePlane>> = None;
    let mut this_background_map: Option<Rc<BackgroundMap>> = None;
    let mut this_background_state: Option<BackgroundState> = None;
    let mut this_sprite_state: Option<Rc<SpriteState>> = None;
    for y in 0..240 {
        sprite_cache = [None; SPRITE_CACHE];
        window_1_cache = [mask16x8::default(); 240 / 8];
        window_2_cache = [mask16x8::default(); 240 / 8];

        for command in &command_buffer.0 {
            match command {
                Command::BindMainState(main_state) => {
                    this_main_state = Some(*main_state);
                },
                Command::BindCMathState(cmath_state) => this_cmath_state = Some(*cmath_state),
                Command::BindBackgroundPlane(background_plane) => {
                    this_background_plane = Some(background_plane.clone());
                },
                Command::BindSpritePlane(sprite_plane) => {
                    this_sprite_plane = Some(sprite_plane.clone());
                },
                Command::BindBackgroundMap(background_map) => {
                    this_background_map = Some(background_map.clone());
                },
                Command::BindBackgroundState(background_state) => {
                    this_background_state = Some(*background_state);
                },
                Command::BindSpriteState(sprite_state) => {
                    sprite_cache = [None; SPRITE_CACHE];
                    this_sprite_state = Some(sprite_state.clone());
                },
                Command::ApplyHDMA(table) => {
                    command_apply_hdma(
                        this_main_state.as_mut(),
                        this_cmath_state.as_mut(),
                        this_background_state.as_mut(),
                        table,
                        y,
                    );
                },
                Command::BeginFrame => {
                    command_begin_frame(
                        this_main_state.as_ref(),
                        &mut main_screen,
                        &mut sub_screen,
                        &window_1_cache,
                        &window_2_cache,
                    );
                },
                Command::UpdateWindows => {
                    command_update_windows(
                        this_main_state.as_ref(),
                        &mut window_1_cache,
                        &mut window_2_cache,
                    );
                },
                Command::DrawBackground => {
                    command_draw_background(
                        this_background_state.as_ref(),
                        this_background_map.as_ref(),
                        this_background_plane.as_ref(),
                        &mut main_screen,
                        &mut sub_screen,
                        &window_1_cache,
                        &window_2_cache,
                        y,
                    );
                },
                Command::DrawSprites(user_type) => {
                    handle_sprite_cache(
                        this_sprite_state.as_ref(),
                        y,
                        &mut sprite_cache,
                        *user_type,
                    );
                    command_draw_sprites(
                        this_sprite_plane.as_ref(),
                        &sprite_cache,
                        &mut main_screen,
                        &mut sub_screen,
                        &window_1_cache,
                        &window_2_cache,
                        y,
                    );
                },
                Command::EndFrame => {
                    command_end_frame(
                        this_main_state.as_ref(),
                        this_cmath_state.as_ref(),
                        &mut main_screen,
                        &mut sub_screen,
                    );
                },
            }
        }

        for x in (0..240).step_by(8).rev() {
            screen[y as usize][x..x + 8].clone_from_slice(main_screen[x / 8].as_array());
        }
    }
}
