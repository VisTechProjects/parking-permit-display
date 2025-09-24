#!/usr/bin/env python3
"""
Render an e-ink-ready BMP of a Toronto parking permit stub.

Now supports choosing the barcode symbology and value:
- Default encodes the **numeric part** of the permit (e.g., 6103268) as **Code 39** (matches what some apps detect).
- You can switch to Code 128 and/or override the barcode payload.

Install:
  pip install pillow python-barcode setuptools

Examples:
  # 2.9" panel; encode Code39 with numeric permit (6103268), print stub 00435
  python render_permit_flexible_barcode.py --size 296x128 --out permit.bmp \
    --plate CSEB187 --permit T6103268 --stub 00435

  # Force Code128 and encode full alphanumeric permit (T6103268)
  python render_permit_flexible_barcode.py --size 296x128 --out permit128.bmp \
    --sym code128 --barcode-value T6103268 --plate CSEB187 --permit T6103268 --stub 00435
"""

from dataclasses import dataclass
from typing import Tuple, Optional
from PIL import Image, ImageDraw, ImageFont
import argparse, os, re

# python-barcode
import barcode
from barcode.writer import ImageWriter

TITLE          = "TEMP PERMIT"
MARGIN_X       = 10
MARGIN_TOP     = 4
MARGIN_BOTTOM  = 4
LINE_SPACING   = 2
BARCODE_HEIGHT_PX_296x128 = 40

FONT_SMALL = ImageFont.load_default()
FONT_MED   = ImageFont.load_default()

@dataclass
class PermitInfo:
    plate: str
    permit: str            # printed in text lines
    valid_from: str
    valid_to: str
    issued: Optional[str]
    stub_digits: str       # printed under bars
    barcode_value: str     # payload actually encoded in bars
    symbology: str         # "code39" or "code128"

def _draw_text_center(draw: ImageDraw.ImageDraw, x: int, y: int, text: str, font) -> int:
    bbox = draw.textbbox((0, 0), text, font=font)
    w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
    draw.text((x - w // 2, y), text, fill=0, font=font)
    return h

def _make_barcode_object(payload: str, symbology: str, checksum: bool=False):
    sym = symbology.lower()
    if sym == "code39" or sym == "code_39":
        from barcode.codex import Code39
        return Code39(payload, writer=ImageWriter(), add_checksum=checksum)
    elif sym == "code128" or sym == "code_128":
        from barcode.code128 import Code128
        return Code128(payload, writer=ImageWriter())
    else:
        # fallback to generic (if python-barcode supports it)
        return barcode.get(sym, payload, writer=ImageWriter())

def generate_barcode_fitted(payload: str, target_width: int, target_height: int,
                            symbology: str="code39", dpi: int = 300) -> Image.Image:
    """
    Generate a barcode image (Code39/Code128) whose width is close to target_width (± ~10 px)
    so we can paste it without resampling (keeps bars crisp).
    """
    module_width = 0.6
    quiet_modules = 10
    last_img_path = None
    for _ in range(12):
        opts = {
            "module_width": module_width,
            "module_height": max(18, target_height),
            "quiet_zone": quiet_modules * module_width,
            "write_text": False,
            "font_size": 0,
            "text_distance": 0,
            "dpi": dpi,
        }
        code = _make_barcode_object(payload, symbology)
        tmp_base = "_barcode_tmp"
        file_path = code.save(tmp_base, opts)  # -> _barcode_tmp.png
        last_img_path = file_path
        img = Image.open(file_path).convert("1")
        w, _ = img.size

        if abs(w - target_width) <= 10:
            try: os.remove(file_path)
            except Exception: pass
            return img

        if w > 0:
            module_width *= (target_width / w)
        module_width = max(0.25, min(module_width, 2.5))

    if last_img_path:
        try: os.remove(last_img_path)
        except Exception: pass
    return img

def render_permit(info: PermitInfo, size: Tuple[int, int], out_path: str):
    w, h = size
    img = Image.new("1", (w, h), 1)
    draw = ImageDraw.Draw(img)

    y = MARGIN_TOP
    y += _draw_text_center(draw, w // 2, y, TITLE, FONT_SMALL) + 6

    left = MARGIN_X
    lines = [
        f"Plate: {info.plate}",
        f"Permit: {info.permit}",
        f"Valid from: {info.valid_from}",
        f"Valid to:   {info.valid_to}",
    ]
    if info.issued:
        lines.append(f"Issued: {info.issued}")

    for line in lines:
        draw.text((left, y), line, fill=0, font=FONT_SMALL)
        lh = draw.textbbox((0, 0), line, font=FONT_SMALL)[3]
        y += lh + LINE_SPACING

    base_bar_h = BARCODE_HEIGHT_PX_296x128 if w >= 290 else int(BARCODE_HEIGHT_PX_296x128 * 0.8)
    reserved_h = base_bar_h + 14 + MARGIN_BOTTOM
    barcode_top = max(y + 2, h - reserved_h)

    target_width = w - (MARGIN_X * 2)
    code_img = generate_barcode_fitted(info.barcode_value, target_width, base_bar_h, symbology=info.symbology, dpi=300)

    bw, bh = code_img.size
    bx = (w - bw) // 2
    by = barcode_top
    img.paste(code_img, (bx, by))

    _draw_text_center(draw, w // 2, by + bh + 2, info.stub_digits, FONT_MED)

    img.save(out_path, format="BMP")

def main():
    ap = argparse.ArgumentParser(description="Render e-ink permit BMP with selectable barcode symbology/value.")
    ap.add_argument("--size", default="296x128", help="Canvas size, e.g., 296x128 or 250x122")
    ap.add_argument("--out", default="permit.bmp", help="Output BMP path")
    ap.add_argument("--sym", default="code39", help="Barcode symbology: code39 or code128")
    ap.add_argument("--barcode-value", default=None, help="Value encoded in the bars. Default: numeric part of --permit for code39; full --permit for code128.")
    ap.add_argument("--plate", default="CSEB187")
    ap.add_argument("--permit", default="T6103268")
    ap.add_argument("--stub", default="00435", help="Short digits printed under the bars")
    ap.add_argument("--from_", dest="valid_from", default="Sep 05, 2025 01:08 AM")
    ap.add_argument("--to", dest="valid_to", default="Sep 12, 2025 01:08 AM")
    ap.add_argument("--issued", default="Sep 05, 2025 01:08 AM")
    args = ap.parse_args()

    if "x" not in args.size:
        raise SystemExit("Size must be like 296x128 or 250x122")
    W, H = map(int, args.size.lower().split("x"))

    # Choose default barcode payload based on symbology
    sym = args.sym.lower()
    if args.barcode_value:
        payload = args.barcode_value
    else:
        if sym == "code39":
            # Many readers report Code 39 with only numerics for these permits
            payload = re.sub(r"\D", "", args.permit)  # numeric part only
        else:
            # Code128 can encode full alphanumeric
            payload = args.permit

    info = PermitInfo(
        plate=args.plate,
        permit=args.permit,
        valid_from=args.valid_from,
        valid_to=args.valid_to,
        issued=args.issued,
        stub_digits=args.stub,
        barcode_value=payload,
        symbology=sym,
    )

    render_permit(info, (W, H), args.out)
    print(f"Saved {args.out} ({W}x{H}) | Symbology: {sym} | Encoded: {payload} | Printed stub: {args.stub}")

if __name__ == "__main__":
    main()
