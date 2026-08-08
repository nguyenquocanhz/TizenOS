#!/usr/bin/env python3
"""
TizenOS GRUB Theme Asset Generator
Tạo hình nền 1920x1080, selection highlight, và biểu tượng (icons) cho menu boot GRUB.
"""

import sys
import os

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8')

try:
    from PIL import Image, ImageDraw, ImageFilter
except ImportError:
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "Pillow"])
    from PIL import Image, ImageDraw, ImageFilter

THEME_DIR = os.path.dirname(os.path.abspath(__file__))

def create_background():
    """Tạo hình nền 1920x1080 với gradient tối và hiệu ứng Glassmorphism Tizen Blue"""
    width, height = 1920, 1080
    img = Image.new("RGBA", (width, height), (10, 15, 29, 255))
    draw = ImageDraw.Draw(img)

    # Radial/Linear gradient: Tối dần từ trên xuống dưới với Tizen Blue glow ở trung tâm
    for y in range(height):
        r = int(10 + (20 - 10) * (y / height))
        g = int(15 + (40 - 15) * (y / height))
        b = int(29 + (75 - 29) * (y / height))
        draw.line([(0, y), (width, y)], fill=(r, g, b, 255))

    # Glow effect ở trung tâm menu boot
    glow = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow)
    glow_draw.ellipse([width//2 - 500, height//2 - 350, width//2 + 500, height//2 + 350],
                      fill=(0, 163, 255, 25))
    glow = glow.filter(ImageFilter.GaussianBlur(120))
    img = Image.alpha_composite(img, glow)

    # Khung Glassmorphism Card cho boot menu ở giữa (left 23%, top 25%, width 54%, height 51%)
    card_box = [int(width * 0.23), int(height * 0.25), int(width * 0.77), int(height * 0.76)]
    card_overlay = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    card_draw = ImageDraw.Draw(card_overlay)
    card_draw.rounded_rectangle(card_box, radius=16, fill=(15, 23, 42, 180), outline=(0, 163, 255, 100), width=2)
    img = Image.alpha_composite(img, card_overlay)
    draw = ImageDraw.Draw(img)

    # Vùng header logo TizenOS Hexagon Icon
    cx, cy = width // 2, int(height * 0.07)
    r = 24
    hex_pts = [
        (cx, cy - r),
        (cx + int(r * 0.866), cy - r // 2),
        (cx + int(r * 0.866), cy + r // 2),
        (cx, cy + r),
        (cx - int(r * 0.866), cy + r // 2),
        (cx - int(r * 0.866), cy - r // 2)
    ]
    draw.polygon(hex_pts, fill=(0, 163, 255, 255), outline=(255, 255, 255, 255), width=2)

    bg_path = os.path.join(THEME_DIR, "background.png")
    img.save(bg_path, "PNG")
    print(f"[OK] Created GRUB background: {bg_path}")

def create_selection_box():
    """Tạo các file slice cho selection box item"""
    w, h = 400, 48
    sel = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(sel)
    draw.rounded_rectangle([0, 0, w-1, h-1], radius=8, fill=(0, 105, 180, 220), outline=(0, 163, 255, 255), width=2)
    
    sel.save(os.path.join(THEME_DIR, "select_bkg_c.png"), "PNG")
    sel.crop((0, 0, 8, 8)).save(os.path.join(THEME_DIR, "select_bkg_nw.png"), "PNG")
    sel.crop((w-8, 0, w, 8)).save(os.path.join(THEME_DIR, "select_bkg_ne.png"), "PNG")
    sel.crop((0, h-8, 8, h)).save(os.path.join(THEME_DIR, "select_bkg_sw.png"), "PNG")
    sel.crop((w-8, h-8, w, h)).save(os.path.join(THEME_DIR, "select_bkg_se.png"), "PNG")
    sel.crop((8, 0, w-8, 8)).save(os.path.join(THEME_DIR, "select_bkg_n.png"), "PNG")
    sel.crop((8, h-8, w-8, h)).save(os.path.join(THEME_DIR, "select_bkg_s.png"), "PNG")
    sel.crop((0, 8, 8, h-8)).save(os.path.join(THEME_DIR, "select_bkg_w.png"), "PNG")
    sel.crop((w-8, 8, w, h-8)).save(os.path.join(THEME_DIR, "select_bkg_e.png"), "PNG")
    print("[OK] Created 9-slice selection box PNGs")

def create_icon(name, color, symbol="app"):
    """Tạo icon 32x32 cho boot menu item"""
    size = 32
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    draw.ellipse([2, 2, 29, 29], fill=color, outline=(255, 255, 255, 220), width=1)

    if symbol == "wayland":
        draw.line([(8, 11), (12, 22), (16, 14), (20, 22), (24, 11)], fill=(255, 255, 255, 255), width=2)
    elif symbol == "safe":
        draw.polygon([(16, 7), (24, 11), (24, 19), (16, 25), (8, 19), (8, 11)], fill=(255, 255, 255, 255))
    elif symbol == "installer":
        draw.line([(16, 8), (16, 20)], fill=(255, 255, 255, 255), width=2)
        draw.polygon([(11, 16), (21, 16), (16, 22)], fill=(255, 255, 255, 255))
    elif symbol == "memtest":
        draw.rectangle([8, 10, 24, 22], fill=None, outline=(255, 255, 255, 255), width=2)
        draw.line([(10, 22), (10, 25)], fill=(255, 255, 255, 255), width=2)
        draw.line([(14, 22), (14, 25)], fill=(255, 255, 255, 255), width=2)
        draw.line([(18, 22), (18, 25)], fill=(255, 255, 255, 255), width=2)
        draw.line([(22, 22), (22, 25)], fill=(255, 255, 255, 255), width=2)
    elif symbol == "reboot":
        draw.arc([8, 8, 24, 24], start=45, end=315, fill=(255, 255, 255, 255), width=2)
    elif symbol == "shutdown":
        draw.arc([8, 10, 24, 26], start=40, end=320, fill=(255, 255, 255, 255), width=2)
        draw.line([(16, 6), (16, 15)], fill=(255, 255, 255, 255), width=2)

    icon_path = os.path.join(THEME_DIR, f"{name}.png")
    img.save(icon_path, "PNG")
    print(f"[OK] Created icon: {name}.png")

if __name__ == "__main__":
    create_background()
    create_selection_box()
    create_icon("tizenos", (0, 105, 180, 230), "wayland")
    create_icon("safe", (180, 100, 0, 230), "safe")
    create_icon("installer", (0, 150, 80, 230), "installer")
    create_icon("memtest", (100, 60, 180, 230), "memtest")
    create_icon("reboot", (70, 70, 70, 230), "reboot")
    create_icon("shutdown", (180, 40, 40, 230), "shutdown")
    print("=== Successfully generated all GRUB Theme assets! ===")
