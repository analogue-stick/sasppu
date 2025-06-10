# SASPPU

_"Super Analogue Stick Picture Processing Unit"_ (SASPPU) is a custom graphics engine for the EMF Tildagon that can entirely bypass the default graphics api uctx, all the way down to flipping to the display. It is inspired by how the SNES and GBA "PPU"s draw to the screen, and is written in a mixture of C and _inline esp32s3 SIMD assembly (!)_. It supports two tiled backgrounds, up to 256 sprites, HDMA (scanline tricks, like Amiga COPPER), and full per pixel "color math" and "window" effects.

## A feature list:
* Full 240x240 resolution with RGB555 (15bit) colour.
* Two 512x512 pixel backgrounds, made up from up to 2048 8x8 tiles.
* Ability to flip background tiles in the x and y axis, or both.
* Infinite per pixel background scrolling.
* Up to 256 sprites, of any size (as long as they are a multiple of 8 wide), of which you can get 16 on any one scanline.
* A dedicated 256x256 pixel area for sprite graphics, separate from the background graphics.
* Ability to flip each sprite individually in the x and y axis, or both.
* Ability to double the size of each sprite individually (in both axes at once), with nearest-neighbour upscale.
* Position sprites anywhere on screen, including offscreen, and either in front or behind the higher background.
* Well defined sprite layering in relation to each other.
* Disable the background and sprite layers if they are not needed.
* Colour 0 is transparent and allows backgrounds to have holes and sprites to not be squares.
* Both a main screen and a sub screen, which can be combined into one as a final step using per pixel mathematical operations.
* Changable solid background colour for both main and sub screen.
* Ability to send each background and individual sprite to either the main screen or the sub screen, or both.
* Two "windows", which allow you to mask off parts of the screen using boolean operations.
* Per background and individual sprite window settings.
* Dedicated fade to/from black feature.
* HDMA: Per scanline adjustment of all registers (including all sprites), so you can perform fancy wavy effects and copper bars.
* Easily fits into flash alongside the firmware (~70kb or so), and doesn't take up that much ram (surprisingly), with the large structures residing in PSRAM.
* Significantly faster than uctx: easily hits 30fps, and on simple scenes can reach 60fps (!), even with the entire rest of the OS running.
* Uses inline esp32s3 SIMD assembly to render 8 pixels at once.
* Co-exists with uctx. SASPPU and uctx are chosen on a per frame basis, and switching in and out of a SASPPU app is seamless.
* Comes with some helper functions for copying data around the graphics memory, including simple graphics decompression, and routines to draw text to the backgrounds.
* Deterministic. HDMA notwithstanding, if you call "SASPPU_render" twice you should get the same frame twice.
* Renders directly into the Tildagon framebuffer, with no double buffering - but also no tearing :)

## Caveats:
* Much harder to use than uctx. You can draw a line with uctx with a couple lines of code, but SASPPU requires much more effort to get graphics on screen.
* Not yet documented. It's probably indecipherable at the moment to anyone who is not me.
* Due to the statically allocated memory buffers that SASPPU uses, only one app can safely use it at once - including minimised apps. If another app tries to start the scheduler refuses to load it.
* An app has to pick either uctx or SASPPU. It cannot currently switch between, and either way it is not possible to use them in the same frame.
* Due to the two caveats above, notifications and other overlay layers are not currently supported.
* uctx specialises in vector graphics, and SASPPU specialises in bitmap graphics. Trying to do the opposite in each is not easy/performant/both.
* No palette, and therefore no oldschool palette effects. Memory bandwidth isn't what it used to be, sorry.
* No arbitrary rotation or scaling (or SNES mode 7) for the same reason as above.
* No anti-aliasing. But then again, that's at least slightly an aesthetic decision :)

## Todo:
* Better docs. God it needs documentation.
* More helper functions written in C. I'm currently just adding them as I need them.
* Integration into the Tildagon Simulator. So far I've managed to get it to compile to WASM (like ctx is).
* Optimise the micropython interface. It's currently a large part of the frame time.
* Eventually, merge upstream so that any Tildagon app can use it (if they'll let me, of course).

## Stretch Goals:
* Port to the Flow3r badge.
* Port to RP2350 (ARM NEON).
* Use it to make a sequel to Badgemon for EMF 2026.
* take over the world. by 2035 all computers will be running SASPPU for rendering to the screen.

Code is publicly available RIGHT NOW at https://github.com/analogue-stick/badge-2024-software/tree/sasppu:
* Main SASPPU C code in components/sasppu
* Micropython wrappers to the C functions in drivers/sasppu
* Random other stuff strewn all over the shop
* It should even compile!

If you're interested a good starting place might be to look at the headers in components/sasppu/include/public/sasppu, but as I said before these are currently undocumented. There is currently a test Tildagon app in the root directory called sasppu_test_cart which may shed some light.
If you know how the SNES, NES or GBA render frames it should be at least somewhat familiar already.

Molive^Analogue_Stick :)
