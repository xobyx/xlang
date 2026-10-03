# Raylib 2D Graphics & Game Engine Examples in xlang

This directory provides complete, runnable games and graphics applications demonstrating the **Raylib C Extension** for `xlang`.

The extension enables high-performance 2D drawing, window management, keyboard/mouse input, and CPU-side offscreen image generation directly from `xlang` scripts across all three execution engines (**Bytecode VM**, **LLVM JIT**, and **Standalone AOT Native Binaries**).

---

## Included Examples

### 1. `dino_run.xb` (Google Chrome Dino Run Arcade Game)
A faithful, polished arcade clone of Google Chrome's beloved T-Rex offline game:
- **T-Rex Character Controller**:
  - Running leg cycle animation alternating at 10 Hz.
  - Smooth jumping physics with gravitational acceleration (`vy = vy + gravity`).
  - Ducking mechanics (lowers hitbox height and switches to elongated running sprite).
  - Dead state with eye "X" on collision.
- **Dynamic Obstacles**:
  - Small Cacti, Tall Cacti, and Multi-Cactus clusters.
  - Flying Pterodactyls (Birds) with animated flapping wings flying at variable altitudes (forcing the player to duck under or jump over).
  - Procedural spacing and cycling.
- **Environmental Dynamics**:
  - Parallax scrolling background clouds.
  - Scrolling ground terrain with animated gravel/pebble texture lines.
  - Dynamic **Day / Night Mode cycle** automatically shifting color schemes every 500 points!
- **Scoring & Difficulty Progression**:
  - Classic 5-digit digital HUD score board with High Score tracker (`HI 00450  00128`).
  - 100-point milestone visual flash celebrations.
  - Progressive game speed acceleration (speed ramps smoothly from 6.5 to 13.5).
- **Controls**:
  - `SPACE` / `UP Arrow` / `Left Mouse Click`: Jump / Start / Restart
  - `DOWN Arrow`: Duck
  - `ESC`: Quit

---

### 2. `bouncing_balls.xb` (Interactive 2D Physics Simulation)
An interactive 2D physics simulation featuring:
- **OOP Modeling**: Uses an `xlang` class `Ball()` with velocity, position, radius, mass, and color attributes.
- **Ball-to-Ball Elastic Collisions**: Pairwise 2D impulse collision resolution with momentum transfer along contact normals and positional overlap separation.
- **Wall Bounces**: Boundary collision detection and velocity reflection.
- **Decorative Render Grid**: Programmatic background grid rasterization using loops and `raylib.draw_line()`.
- **Interactive Input**: Left-click mouse input to teleport the red ball to the cursor position.
- **HUD Panel & Diagnostics**: Live FPS counter and instructions overlay.

---

### 3. `procedural_art.xb` (Headless CPU Image Rasterization & PNG Export)
A non-GUI / headless image generation demo:
- Creates an offscreen CPU image buffer using `raylib.gen_image_color(640, 480, bg_color)`.
- Draws geometric background grid patterns with `raylib.image_draw_line()`.
- Draws concentric circles and accent rectangles using `raylib.image_draw_circle()` and `raylib.image_draw_rectangle()`.
- Renders text onto the image with `raylib.image_draw_text()`.
- Exports the final rasterized image directly to `procedural_art.png` via `raylib.export_image()`.

---

## How to Run

### Method 1: Bytecode Virtual Machine (Instant Startup)
Run directly with zero compilation overhead:
```bash
# Run Google Dino Run
./bin/Release/xlang examples/raylib/dino_run.xb

# Run Bouncing Balls physics simulation
./bin/Release/xlang examples/raylib/bouncing_balls.xb

# Run headless procedural art generator
./bin/Release/xlang examples/raylib/procedural_art.xb
```

### Method 2: Standalone Native AOT Binary (`xlang build`)
Compile into an optimized, standalone native ELF executable:
```bash
# Compile and run Google Dino Run
./bin/Release/xlang build examples/raylib/dino_run.xb -o dino_run
./dino_run

# Compile and run Bouncing Balls
./bin/Release/xlang build examples/raylib/bouncing_balls.xb -o bouncing_balls
./bouncing_balls
```

---

## Prerequisites & Building the Extension

The Raylib extension comes pre-bundled and pre-compiled under `lib/raylib.so` and `examples/c_extension/raylib/raylib.so`.

If you modify `examples/c_extension/raylib/xraylib.c`, recompile it using:
```bash
make -C examples/c_extension/raylib
```

### System Dependencies on Linux:
- `libGL` (Mesa OpenGL runtime)
- `libX11` (X Window System)
- `pthread`, `dl`, `m`
