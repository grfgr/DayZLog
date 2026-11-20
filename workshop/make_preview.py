import math, random, sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter

S = 1024
random.seed(7)
F_TITLE = "/usr/share/fonts/opentype/inter/InterDisplay-Bold.otf"
F_BLACK = "/usr/share/fonts/opentype/inter/Inter-ExtraBold.otf"
F_MONO = "/usr/share/fonts/truetype/jetbrains-mono/JetBrainsMono-Bold.ttf"

# background: dark olive radial gradient
bg = Image.new("RGB", (S, S))
px = bg.load()
cx, cy = S * 0.5, S * 0.42
for y in range(S):
    for x in range(S):
        d = math.hypot(x - cx, y - cy) / (S * 0.75)
        t = max(0.0, 1.0 - d)
        r = int(10 + 30 * t); g = int(13 + 36 * t); b = int(11 + 24 * t)
        px[x, y] = (r, g, b)

# faint log lines
logs = Image.new("RGBA", (S, S), (0, 0, 0, 0))
ld = ImageDraw.Draw(logs)
mono = ImageFont.truetype(F_MONO, 19)
names = ["Bandit", "Survivor", "Mirek", "Boris", "Kolya", "Denis", "Taiki", "Lewis"]
guns = ["M4-A1", "AKM", "SVD", "Mosin 91/30", "FX-45", "VSS", "KA-M", "Kitchen Knife"]
zones = ["Head", "Torso", "LeftLeg", "RightArm", "Torso", "Head"]
y = 14
while y < S:
    h, m, s_ = random.randint(0, 23), random.randint(0, 59), random.randint(0, 59)
    kind = random.choice(["HIT ", "HIT ", "HIT ", "KILL"])
    a, v = random.sample(names, 2)
    line = f"[{h:02}:{m:02}:{s_:02}] {kind} | {a} -> {v} | {random.choice(guns)} | {random.choice(zones)} | {random.randint(5,420)}m | <{random.randint(1000,14000)}.{random.randint(0,9)}, {random.randint(1000,14000)}.{random.randint(0,9)}>"
    col = (190, 60, 50, 46) if kind == "KILL" else (120, 170, 110, 34)
    ld.text((18, y), line, font=mono, fill=col)
    y += 30
bg = Image.alpha_composite(bg.convert("RGBA"), logs)

# vignette
vig = Image.new("L", (S, S), 0)
vd = ImageDraw.Draw(vig)
for i in range(60):
    vd.ellipse([-260 + i * 6, -260 + i * 6, S + 260 - i * 6, S + 260 - i * 6], fill=int(i * 4.2))
vig = vig.filter(ImageFilter.GaussianBlur(60))
black = Image.new("RGBA", (S, S), (0, 0, 0, 255))
bg = Image.composite(bg, black, vig)

# reticle
ret = Image.new("RGBA", (S, S), (0, 0, 0, 0))
rd = ImageDraw.Draw(ret)
RED = (214, 38, 32, 255)
rcx, rcy, R = S // 2, 380, 205
rd.ellipse([rcx - R, rcy - R, rcx + R, rcy + R], outline=RED, width=12)
rd.ellipse([rcx - 118, rcy - 118, rcx + 118, rcy + 118], outline=(214, 38, 32, 150), width=4)
gap = 44
for dx, dy in [(1, 0), (-1, 0), (0, 1), (0, -1)]:
    x0, y0 = rcx + dx * gap, rcy + dy * gap
    x1, y1 = rcx + dx * (R + 46), rcy + dy * (R + 46)
    rd.line([x0, y0, x1, y1], fill=RED, width=10)
for k in range(1, 4):  # mil ticks
    for sgn in (1, -1):
        tx = rcx + sgn * (gap + k * 42)
        rd.line([tx, rcy - 16, tx, rcy + 16], fill=RED, width=5)
        ty = rcy + sgn * (gap + k * 42)
        rd.line([rcx - 16, ty, rcx + 16, ty], fill=RED, width=5)
rd.ellipse([rcx - 10, rcy - 10, rcx + 10, rcy + 10], fill=RED)
glow = ret.filter(ImageFilter.GaussianBlur(14))
bg = Image.alpha_composite(bg, glow)
bg = Image.alpha_composite(bg, ret)

# title
d = ImageDraw.Draw(bg)
title_font = ImageFont.truetype(F_BLACK, 168)
title = "PvP LOG"
tw = d.textlength(title, font=title_font)
tx, ty = (S - tw) / 2, 640
shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
ImageDraw.Draw(shadow).text((tx + 6, ty + 8), title, font=title_font, fill=(0, 0, 0, 230))
bg = Image.alpha_composite(bg, shadow.filter(ImageFilter.GaussianBlur(6)))
d = ImageDraw.Draw(bg)
d.text((tx, ty), title, font=title_font, fill=(236, 232, 220, 255))
sub_font = ImageFont.truetype(F_TITLE, 44)
sub = "HITS  ·  KILLS  ·  COORDS"
sw = d.textlength(sub, font=sub_font)
d.text(((S - sw) / 2, ty + 196), sub, font=sub_font, fill=(214, 38, 32, 255))

# film grain
grain = Image.effect_noise((S, S), 22).convert("L")
grain = Image.merge("RGBA", (grain, grain, grain, Image.new("L", (S, S), 20)))
bg = Image.alpha_composite(bg, grain)

# corner ribbon, lower-left, text rising at 45 degrees
band_w, band_h = 640, 84
band = Image.new("RGBA", (band_w, band_h), (0, 0, 0, 0))
bd = ImageDraw.Draw(band)
bd.rectangle([0, 0, band_w, band_h], fill=(214, 38, 32, 255))
bd.rectangle([0, 5, band_w, 8], fill=(20, 16, 14, 120))
bd.rectangle([0, band_h - 9, band_w, band_h - 6], fill=(20, 16, 14, 120))
rib_font = ImageFont.truetype(F_MONO, 42)
text = "ZloyDed.dll"
rw = bd.textlength(text, font=rib_font)
bbox = bd.textbbox((0, 0), text, font=rib_font)
bd.text(((band_w - rw) / 2, (band_h - (bbox[3] - bbox[1])) / 2 - bbox[1]), text, font=rib_font, fill=(245, 240, 230, 255))
rot = band.rotate(-45, expand=True, resample=Image.BICUBIC)
off = 150  # distance of band centre from the corner along each axis
px_, py_ = off - rot.width // 2, S - off - rot.height // 2
rib_shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
sh = Image.new("RGBA", rot.size, (0, 0, 0, 0))
sh.putalpha(rot.split()[3].point(lambda a: int(a * 0.7)))
rib_shadow.paste(sh, (px_ + 6, py_ + 6), sh)
bg = Image.alpha_composite(bg, rib_shadow.filter(ImageFilter.GaussianBlur(8)))
layer = Image.new("RGBA", (S, S), (0, 0, 0, 0))
layer.paste(rot, (px_, py_), rot)
bg = Image.alpha_composite(bg, layer)

out = bg.convert("RGB")
out.save(sys.argv[1] + "/preview.jpg", quality=90, optimize=True)
out.resize((512, 512), Image.LANCZOS).save(sys.argv[1] + "/preview_512.jpg", quality=92, optimize=True)
