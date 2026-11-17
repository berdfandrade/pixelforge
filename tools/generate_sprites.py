import pathlib
import sys

OUT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "assets/sprites")


def write(name, width, height, palette, animations):
    lines = [f"size {width} {height}"]
    lines += [f"color {key} {value}" for key, value in palette.items()]
    for anim_name, fps, frames in animations:
        lines.append(f"anim {anim_name} {fps}")
        for frame in frames:
            assert len(frame) == height, (name, anim_name, len(frame))
            for row in frame:
                assert len(row) == width, (name, anim_name, row, len(row))
            lines.append("frame")
            lines += frame
    (OUT / f"{name}.sprite").write_text("\n".join(lines) + "\n")


HERO_TOP = [
    "...kkkkkk...",
    "..kooooookk.",
    ".kooooooodk.",
    ".kodssssssk.",
    ".kdsesssesk.",
    ".kssssssssk.",
    "..ksssssk...",
    "..kkttttkk..",
    ".kttttttttk.",
    "kstttttttusk",
    ".kuttttttuk.",
    "..kppppppk..",
]
HERO_BLINK = HERO_TOP[:4] + [".kdsssssssk."] + HERO_TOP[5:]
LEGS_IDLE = ["..kppkkppk..", "..kpk..kpk..", "..kbk..kbk..", ".kbbk..kbbk."]
LEGS_SPREAD = ["..kppkkppk..", ".kpk....kpk.", ".kbk....kbk.", "kbbk....kbbk"]
LEGS_PASS = ["..kppppppk..", "...kpppk....", "...kbbbk....", "...kbbbbk..."]
LEGS_JUMP = ["..kppkkppk..", ".kpk...kbbk.", ".kbbk..kkk..", "..kk........"]

write("hero", 12, 16,
      {"k": "#1a1c22", "o": "#ff7a3d", "d": "#c4532a", "s": "#f2c29b", "e": "#1a1c22",
       "t": "#4fd1c5", "u": "#2e9e94", "p": "#3b3f52", "b": "#22242e"},
      [("idle", 4, [HERO_TOP + LEGS_IDLE] * 3 + [HERO_BLINK + LEGS_IDLE]),
       ("run", 10, [HERO_TOP + LEGS_SPREAD, HERO_TOP + LEGS_PASS]),
       ("jump", 1, [HERO_TOP + LEGS_JUMP])])

SLIME = [
    ".....kkkkkk.....",
    "...kkvvvvvvkk...",
    "..kvllvvvvvvvk..",
    ".kvllvvvvvvvvvk.",
    ".kvvvkvvvvkvvvk.",
    "kvvvvkvvvvkvvvvk",
    "kvvvvvvvvvvvvvvk",
    "kvvvvvvvvvvvvddk",
    "kddvvvvvvvvvdddk",
    ".kkkkkkkkkkkkkk.",
]
SLIME_SQUASH = ["................", "...kkkkkkkkkk..."] + SLIME[2:]
write("slime", 16, 10,
      {"k": "#1a1c22", "v": "#9d7bff", "l": "#c9b6ff", "d": "#6b4fd1"},
      [("idle", 3, [SLIME, SLIME_SQUASH])])

COIN_FULL = ["..kkkk..", ".kyyyyk.", "kyhyyyyk", "kyhyydyk", "kyyyydyk", "kyyyyddk", ".kyddyk.", "..kkkk.."]
COIN_NARROW = ["...kk...", "..kyyk..", "..khyk..", "..kyyk..", "..kyyk..", "..kydk..", "..kydk..", "...kk..."]
COIN_EDGE = ["...kk..."] * 8
write("coin", 8, 8,
      {"k": "#1a1c22", "y": "#ffc94d", "h": "#fff1b8", "d": "#c98a1e"},
      [("idle", 8, [COIN_FULL, COIN_NARROW, COIN_EDGE, COIN_NARROW])])


def dirt_rows(width, height, seed, dark):
    base, shade = ("R", "r") if dark else ("r", "R")
    rows = []
    for y in range(height):
        row = ""
        for x in range(width):
            value = (x * 7 + y * 13 + seed * 5) % 23
            row += "p" if value == 0 else shade if value in (5, 11) else base
        rows.append(row)
    return rows


def grass_tile(light, dark_dirt):
    top = ("g" if light else "G") * 16
    edge = "".join("e" if x % 5 in (1, 2) else ("g" if light else "G") for x in range(16))
    return [top] * 3 + [edge, "e" * 16] + dirt_rows(16, 11, 1, dark_dirt)


TERRAIN = {"g": "#6aa874", "G": "#4f8c5d", "e": "#3e6e4a", "r": "#6b4630", "R": "#573826", "p": "#7d5740"}
write("grass", 16, 16, TERRAIN, [("idle", 1, [grass_tile(True, False)]), ("alt", 1, [grass_tile(False, True)])])
write("dirt", 16, 16, TERRAIN, [("idle", 1, [dirt_rows(16, 16, 2, False)]), ("alt", 1, [dirt_rows(16, 16, 3, True)])])

platform_top = "".join("g" if (x // 8) % 2 == 0 else "G" for x in range(48))
platform_edge = "".join("e" if x % 6 in (2, 3) else platform_top[x] for x in range(48))
write("platform", 48, 14, TERRAIN,
      [("idle", 1, [[platform_top] * 3 + [platform_edge, "e" * 48] + dirt_rows(48, 9, 4, False)])])


def meadow(seed, dark):
    base, tuft = ("m", "M") if not dark else ("M", "m")
    rows = []
    for y in range(16):
        row = ""
        for x in range(16):
            value = ((x * 73856093) ^ (y * 19349663) ^ (seed * 83492791)) % 31
            row += "f" if value == 0 else "t" if value in (4, 17) else tuft if value in (9, 22) else base
        rows.append(row)
    return rows


MEADOW = {"m": "#5a9a5f", "M": "#4f8c55", "t": "#3f7448", "f": "#e8d27a"}
write("floor", 16, 16, MEADOW, [("idle", 1, [meadow(1, False)]), ("alt", 1, [meadow(2, True)])])


def stone_top():
    rows = ["h" * 16]
    for y in range(1, 15):
        rows.append("h" + "".join("c" if (x * 5 + y * 3) % 13 == 0 else "s" for x in range(1, 15)) + "d")
    rows.append("d" * 16)
    return rows


def brick_face():
    rows = ["h" * 16, "s" * 16, "d" * 16]
    for y in range(3, 16):
        if (y - 3) % 4 == 3:
            rows.append("o" * 16)
            continue
        offset = 0 if ((y - 3) // 4) % 2 == 0 else 4
        rows.append("".join("o" if (x + offset) % 8 == 7 else ("b" if ((x + offset) // 8) % 2 == 0 else "B") for x in range(16)))
    return rows


STONE = {"h": "#9a9fb6", "s": "#6f7490", "c": "#5a5f78", "d": "#4b4f63", "o": "#2f3242", "b": "#7a5a48", "B": "#8a6a56"}
write("wall", 16, 16, STONE, [("idle", 1, [stone_top()]), ("face", 1, [brick_face()])])


def diamond_span(row, width=32, height=16):
    half = height // 2
    reach = 2 * (row + 1) if row < half else 2 * (height - row)
    center = width // 2
    return center - reach, center + reach


def iso_floor(light):
    base, accent = ("g", "G") if light else ("G", "g")
    rows = []
    for y in range(16):
        start, end = diamond_span(y)
        row = ""
        for x in range(32):
            if not start <= x < end:
                row += "."
            elif y >= 8 and (x == start or x == end - 1):
                row += "e"
            else:
                row += accent if (x * 7 + y * 5) % 17 == 0 else base
        rows.append(row)
    return rows


ISO = {"g": "#6aa874", "G": "#5a9a65", "e": "#3e6e4a", "s": "#9a9fb6", "h": "#b8bccf", "l": "#4b4f63", "r": "#6f7490", "k": "#2f3242"}
write("iso_floor", 32, 16, ISO, [("idle", 1, [iso_floor(True)]), ("alt", 1, [iso_floor(False)])])


def iso_block():
    rise = 8
    rows = [["."] * 32 for _ in range(16 + rise)]
    for y in range(16):
        start, end = diamond_span(y)
        for x in range(start, end):
            edge = y < 8 and (x == start or x == end - 1)
            rows[y][x] = "h" if edge else "s"
    for x in range(32):
        lower_edge = 8 + x // 2 if x < 16 else 8 + (31 - x) // 2
        for y in range(lower_edge + 1, lower_edge + rise):
            rows[y][x] = "l" if x < 16 else "r"
        rows[lower_edge + rise][x] = "k"
    return ["".join(row) for row in rows]


write("iso_block", 32, 24, ISO, [("idle", 1, [iso_block()])])
