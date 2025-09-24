#!/usr/bin/env python3
"""
Toronto Parking Permit Barcode Generator
Recreates the barcode from permit T6103268
"""

import os
from PIL import Image, ImageDraw, ImageFont

try:
    from barcode import Code128
    from barcode.writer import ImageWriter
    BARCODE_LIB_AVAILABLE = True
except ImportError:
    BARCODE_LIB_AVAILABLE = False
    print("Warning: python-barcode library not found. Install with: pip install python-barcode[images]")

def generate_barcode_with_library(permit_number, filename="parking_permit_barcode.png"):
    """Generate barcode using python-barcode library (recommended)"""
    if not BARCODE_LIB_AVAILABLE:
        print("Please install python-barcode: pip install python-barcode[images]")
        return None
    
    # Create Code128 barcode
    code128 = Code128(permit_number, writer=ImageWriter())
    
    # Configure the barcode appearance
    options = {
        'module_width': 0.3,    # Width of bars
        'module_height': 15.0,  # Height of bars
        'quiet_zone': 6.5,      # Margin around barcode
        'font_size': 10,        # Text size below barcode
        'text_distance': 5.0,   # Distance between bars and text
        'background': 'white',  # Background color
        'foreground': 'black',  # Bar color
    }
    
    # Save the barcode
    filename_without_ext = filename.rsplit('.', 1)[0]
    code128.save(filename_without_ext, options=options)
    
    actual_filename = f"{filename_without_ext}.png"
    print(f"Barcode saved as: {actual_filename}")
    return actual_filename

def generate_simple_barcode(permit_number, filename="simple_barcode.png"):
    """Generate a simple barcode pattern manually (fallback method)"""
    
    # Simple encoding pattern (this is NOT real Code128, just for demonstration)
    # In real Code128, each character has a specific bar pattern
    patterns = {
        '0': [2, 1, 2, 2, 2, 2],
        '1': [2, 2, 2, 1, 2, 2], 
        '2': [2, 2, 2, 2, 2, 1],
        '3': [1, 2, 1, 2, 2, 3],
        '4': [1, 2, 1, 3, 2, 2],
        '5': [1, 3, 1, 2, 2, 2],
        '6': [1, 2, 2, 2, 1, 3],
        '7': [1, 2, 2, 3, 1, 2],
        '8': [1, 3, 2, 2, 1, 2],
        '9': [2, 2, 1, 2, 1, 3],
    }
    
    # Image dimensions
    bar_height = 60
    bar_unit_width = 3
    margin = 20
    
    # Calculate total width
    total_bars = 0
    for digit in permit_number:
        if digit in patterns:
            total_bars += sum(patterns[digit])
    
    img_width = total_bars * bar_unit_width + 2 * margin
    img_height = bar_height + 40  # Extra space for text
    
    # Create image
    img = Image.new('RGB', (img_width, img_height), 'white')
    draw = ImageDraw.Draw(img)
    
    # Draw start pattern
    x = margin
    
    # Draw bars for each digit
    for digit in permit_number:
        if digit in patterns:
            pattern = patterns[digit]
            for i, bar_width in enumerate(pattern):
                if i % 2 == 0:  # Black bars (even indices)
                    draw.rectangle([x, margin, x + bar_width * bar_unit_width, margin + bar_height], 
                                 fill='black')
                x += bar_width * bar_unit_width
    
    # Add text below barcode
    try:
        # Try to use a TrueType font
        font = ImageFont.truetype("arial.ttf", 12)
    except:
        # Fall back to default font
        font = ImageFont.load_default()
    
    text_bbox = draw.textbbox((0, 0), permit_number, font=font)
    text_width = text_bbox[2] - text_bbox[0]
    text_x = (img_width - text_width) // 2
    text_y = margin + bar_height + 10
    
    draw.text((text_x, text_y), permit_number, fill='black', font=font)
    
    # Save image
    img.save(filename)
    print(f"Simple barcode saved as: {filename}")
    return filename

def create_permit_display(permit_data, filename="permit_display.png"):
    """Create a complete permit display similar to the e-ink version"""
    
    # Image dimensions for 2-inch display simulation
    img_width = 250
    img_height = 122
    
    img = Image.new('RGB', (img_width, img_height), 'white')
    draw = ImageDraw.Draw(img)
    
    try:
        title_font = ImageFont.truetype("arial.ttf", 14)
        normal_font = ImageFont.truetype("arial.ttf", 10)
        small_font = ImageFont.truetype("arial.ttf", 8)
    except:
        title_font = ImageFont.load_default()
        normal_font = ImageFont.load_default()
        small_font = ImageFont.load_default()
    
    # Title
    draw.text((5, 5), "PARKING PERMIT", fill='black', font=title_font)
    draw.line([(5, 22), (245, 22)], fill='black', width=1)
    
    # Permit details
    y = 30
    draw.text((5, y), f"Permit: {permit_data['permit_no']}", fill='black', font=normal_font)
    y += 15
    draw.text((5, y), f"Plate: {permit_data['plate_no']}", fill='black', font=normal_font)
    y += 15
    draw.text((5, y), f"Valid: {permit_data['valid_period']}", fill='black', font=small_font)
    y += 12
    draw.text((5, y), f"Location: {permit_data['location']}", fill='black', font=small_font)
    
    # Simple barcode representation
    barcode_y = 85
    barcode_x = 5
    
    # Draw simple bars
    for i, digit in enumerate(permit_data['barcode_data']):
        bar_pattern = int(digit) % 4 + 1  # Simple pattern
        for j in range(bar_pattern):
            if (i + j) % 2 == 0:
                draw.rectangle([barcode_x + i*6 + j*2, barcode_y, 
                              barcode_x + i*6 + j*2 + 1, barcode_y + 20], fill='black')
    
    # Barcode number
    draw.text((barcode_x, barcode_y + 25), permit_data['barcode_data'], fill='black', font=small_font)
    
    # Status
    draw.text((180, barcode_y + 25), "VALID", fill='black', font=normal_font)
    draw.rectangle([175, barcode_y + 10, 240, barcode_y + 35], outline='black', width=1)
    
    img.save(filename)
    print(f"Permit display saved as: {filename}")
    return filename

def main():
    # Parking permit data from the PDF
    permit_data = {
        'permit_no': 'T6103268',
        'plate_no': 'CSEB187',
        'valid_period': 'Sep 05-12, 2025',
        'barcode_data': '6103268',  # The actual barcode content
        'display_num': '00435',     # Display number below barcode
        'location': 'BATAVIA AVE',
        'amount': '$48.38'
    }
    
    print("Toronto Parking Permit Barcode Generator")
    print("=" * 50)
    print(f"Permit Number: {permit_data['permit_no']}")
    print(f"Barcode Data: {permit_data['barcode_data']}")
    print(f"Display Number: {permit_data['display_num']}")
    print()
    
    # Generate barcode using library (if available)
    if BARCODE_LIB_AVAILABLE:
        print("Generating professional Code39 barcode...")
        generate_barcode_with_library(permit_data['barcode_data'])
    else:
        print("Library not available. Install with:")
        print("pip install python-barcode[images] pillow")
    
    # Generate simple Code39 barcode as fallback
    print("Generating simple Code39 barcode pattern...")
    generate_simple_barcode(permit_data['barcode_data'])
    
    # Generate complete permit display
    print("Generating permit display...")
    create_permit_display(permit_data)
    
    print("\nFiles generated successfully!")
    print("- For ESP32: Use the barcode data '6103268' with Code39 library")
    print("- Code39 format: *6103268* (with start/stop characters)")
    print("- The barcode encodes the permit number without the 'T' prefix")

if __name__ == "__main__":
    main()