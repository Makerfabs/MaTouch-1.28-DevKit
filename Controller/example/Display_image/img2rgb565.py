from PIL import Image
import sys

def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert_image_to_c_array(image_path, output_path, width=240, height=240):
    img = Image.open(image_path).resize((width, height))
    pixels = img.convert("RGB").load()

    with open(output_path, "w") as f:
        f.write(f"const uint16_t Image[{width*height}] = {{\n")
        for y in range(height):
            for x in range(width):
                r, g, b = pixels[x, y]
                rgb565 = rgb888_to_rgb565(r, g, b)
                f.write(f"0x{rgb565:04X},")
            f.write("\n")
        f.write("};\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python img2rgb565.py input.png output.h")
        sys.exit(1)
    convert_image_to_c_array(sys.argv[1], sys.argv[2])
