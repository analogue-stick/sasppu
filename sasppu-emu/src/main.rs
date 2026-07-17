#![feature(portable_simd)]
#![feature(iter_array_chunks)]
#![allow(static_mut_refs)]
use std::{
    collections::VecDeque,
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};

pub const BG_DEFAULT: &[u8; 256 * 256 * 2] = include_bytes!("../assets/kodim05.png.bgraw");
pub const SPR_DEFAULT: &[u8; 256 * 32 * 2] = include_bytes!("../assets/sprites.png.bgraw");

use minifb::{Key, Window, WindowOptions};
use sasppu_sys::*;

macro_rules! colour {
    ($r:expr, $g:expr, $b:expr) => {
        ($r << 10) | ($g << 5) | ($b)
    };
}

fn main() {
    let width = 960;
    let height = 960;

    let mut window = Window::new(
        "SASPPU EMU - Press ESC to exit",
        width,
        height,
        WindowOptions::default(),
    )
    .expect("Unable to create the window");

    window.set_target_fps(0);

    const TEST_SPR_COUNT: usize = 32;

    let mut before_buf = [[0u16; 240]; 240];
    let mut buffer = [0u32; 960 * 960];

    let mut main_state = MainState::default();
    main_state.mainscreen_colour = colour!(31, 0, 0);
    main_state.subscreen_colour = colour!(0, 0, 0);
    main_state.window_1_left = 30;
    main_state.window_1_right = 160;
    main_state.window_2_left = 80;
    main_state.window_2_right = 210;
    main_state.flags = MAIN_CMATH_ENABLE;

    let mut bg0_state = BackgroundState::default();
    bg0_state.windows = (WINDOW_A | WINDOW_AB | WINDOW_B) as u8;
    bg0_state.flags = BG_C_MATH;

    let mut cmath_state = CMathState::default();
    cmath_state.flags = CMATH_CMATH_ENABLE | CMATH_SUB_SUB_SCREEN;

    let oam = new_sprite_state_smol();

    for (i, spr) in oam
        .write()
        .unwrap()
        .iter_mut()
        .take(TEST_SPR_COUNT)
        .enumerate()
    {
        spr.flags |= SPR_ENABLED;
        if i & 1 > 0 {
            spr.windows = WINDOW_AB << 4;
        } else {
            spr.windows = WINDOW_X | WINDOW_A | WINDOW_B;
        }
        if (i & 2 > 0) ^ (i & 16 > 0) {
            spr.flags |= SPR_FLIP_X;
        }
        if (i & 4 > 0) ^ (i & 16 > 0) {
            spr.flags |= SPR_FLIP_Y;
        }
        if (i & 8 > 0) ^ (i & 16 > 0) {
            spr.flags |= SPR_DOUBLE;
        }
        spr.width = 32;
        spr.height = 32;
        spr.x = 0;
        spr.graphics_x = ((i as u8 >> 1) % 8) * 4;
    }

    let background = new_background_plane();

    for val in background
        .write()
        .unwrap()
        .iter_mut()
        .zip(BG_DEFAULT.iter().cloned().array_chunks::<16>())
    {
        for v in val
            .0
            .as_mut_array()
            .iter_mut()
            .zip(val.1.into_iter().array_chunks::<2>())
        {
            *v.0 = u16::from_le_bytes(v.1);
        }
    }

    let bg0 = new_background_map();

    let mut bg0_write = bg0.write().unwrap();

    for y in 0..MAP_HEIGHT {
        let (ypos, flipy) = if y >= MAP_HEIGHT / 2 {
            (MAP_HEIGHT - y - 1, true)
        } else {
            (y, false)
        };
        for x in 0..MAP_WIDTH {
            let (xpos, flipx) = if x >= MAP_WIDTH / 2 {
                (MAP_WIDTH - x - 1, true)
            } else {
                (x, false)
            };
            bg0_write[y][x] = (((xpos + (ypos * BG_WIDTH)) * 8) >> 1) as u16
                | ((flipy as u16) << 1)
                | (flipx as u16);
        }
    }

    drop(bg0_write);

    let sprites = new_sprite_plane();

    for val in sprites
        .write()
        .unwrap()
        .iter_mut()
        .flatten()
        .zip(SPR_DEFAULT.iter().cloned().array_chunks::<16>())
    {
        for v in val
            .0
            .as_mut_array()
            .iter_mut()
            .zip(val.1.into_iter().array_chunks::<2>())
        {
            *v.0 = u16::from_le_bytes(v.1);
        }
    }

    let mut i = 0usize;
    let mut times = VecDeque::new();
    while window.is_open() && !window.is_key_down(Key::Escape) {
        {
            let now = Instant::now();
            let epoch = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_nanos() as f64
                / 10000000000.0;
            bg0_state.scroll_x = ((epoch * 2.0).sin() * 256.0 + 256.0) as i16;
            bg0_state.scroll_y = ((epoch * 3.0).cos() * 256.0 + 256.0) as i16;
            for (i, spr) in oam
                .write()
                .unwrap()
                .iter_mut()
                .take(TEST_SPR_COUNT)
                .enumerate()
            {
                spr.x = ((epoch * (5.0 + (0.3 * (i >> 1) as f64))).sin() * (120.0) + (120.0 - 16.0))
                    as i16;
                spr.y = ((epoch * (7.0 + (0.2 * (i >> 1) as f64))).cos() * (120.0) + (120.0 - 16.0))
                    as i16;
            }

            let command_buffer = vec![
                Command::BindMainState(main_state),
                Command::BindCMathState(cmath_state),
                Command::BindBackgroundMap(bg0.clone()),
                Command::BindBackgroundPlane(background.clone()),
                Command::BindBackgroundState(bg0_state),
                Command::BindSpritePlane(sprites.clone()),
                Command::BindSpriteState(oam.clone().into()),
                Command::UpdateWindows,
                Command::BeginFrame,
                Command::DrawBackground,
                Command::DrawSprites(0),
                Command::EndFrame,
            ];
            let command_buffer = CommandBuffer::compile(command_buffer);
            render(&mut before_buf, &command_buffer);

            for (x, col) in buffer.iter_mut().zip(before_buf.iter().flatten()) {
                *x = ((((col >> 11) & 0x1F) as u32) << (16 + 3))
                    | ((((col >> 5) & 0x3F) as u32) << (8 + 2))
                    | (((*col & 0x1F) as u32) << 3);
            }
            for y in (0..240).rev() {
                for x in (0..240).rev() {
                    let val = buffer[(y * 240) + x];
                    for xx in 0..4 {
                        for yy in 0..4 {
                            buffer[(y * 960 * 4 + (960 * yy)) + (x * 4) + (xx)] = val;
                        }
                    }
                }
            }
            let current_time = now.elapsed();
            times.push_back(current_time);
            if times.len() > 500 {
                times.pop_front();
            }
            let elapsed_time: f64 =
                times.iter().sum::<Duration>().as_secs_f64() / times.len() as f64;
            if i & 0xFF == 0 {
                println!(
                    "Frame: {:.4}ms, Avg: {:.4}, Avg. FPS: {:.4}",
                    current_time.as_secs_f64() * 1000.0,
                    elapsed_time * 1000.0,
                    1.0f64 / elapsed_time
                );
            }
            i = i.wrapping_add(1);
            window.update_with_buffer(&buffer, width, height).unwrap();
        }
    }
}
