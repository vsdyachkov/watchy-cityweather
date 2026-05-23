#!/usr/bin/env python3
import argparse
import binascii
from datetime import datetime
from pathlib import Path
import re
import struct
import sys
import time
import zlib


BEGIN_RE = re.compile(r"CW_SCREENSHOT_BEGIN\s+(\d+)\s+(\d+)\s+(\d+)")


def png_chunk(kind, data):
    return (
        struct.pack(">I", len(data))
        + kind
        + data
        + struct.pack(">I", binascii.crc32(kind + data) & 0xFFFFFFFF)
    )


def write_png(path, width, height, packed, scale):
    out_width = width * scale
    out_height = height * scale
    source_row_bytes = (width + 7) // 8
    rows = bytearray()

    for y in range(height):
        expanded = bytearray()
        row = packed[y * source_row_bytes : (y + 1) * source_row_bytes]
        for x in range(width):
            white = (row[x // 8] & (0x80 >> (x & 7))) != 0
            expanded.extend([255 if white else 0] * scale)
        for _ in range(scale):
            rows.append(0)
            rows.extend(expanded)

    ihdr = struct.pack(">IIBBBBB", out_width, out_height, 8, 0, 0, 0, 0)
    payload = (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"IDAT", zlib.compress(bytes(rows), 9))
        + png_chunk(b"IEND", b"")
    )

    with open(path, "wb") as file:
        file.write(payload)


def parse_screenshot_lines(lines):
    width = height = expected_size = None
    hex_chunks = []
    capturing = False

    for raw_line in lines:
        line = raw_line.decode("utf-8", errors="ignore") if isinstance(raw_line, bytes) else raw_line
        if not capturing:
            match = BEGIN_RE.search(line)
            if match:
                width, height, expected_size = map(int, match.groups())
                capturing = True
            continue

        if "CW_SCREENSHOT_END" in line:
            data = bytes.fromhex("".join(hex_chunks))
            if len(data) != expected_size:
                raise ValueError(f"Expected {expected_size} bytes, received {len(data)} bytes")
            return width, height, data

        hex_chunks.append("".join(re.findall(r"[0-9A-Fa-f]{2}", line)))

    raise TimeoutError("Screenshot frame was not received")


def parse_screenshot_from_serial(ser, timeout=None):
    deadline = time.monotonic() + timeout if timeout is not None else None
    lines = []
    while deadline is None or time.monotonic() < deadline:
        line = ser.readline()
        if not line:
            continue
        lines.append(line)
        if b"CW_SCREENSHOT_END" in line:
            return parse_screenshot_lines(lines)

    return parse_screenshot_lines(lines)


def open_serial(port, baud):
    try:
        import serial
    except ImportError as exc:
        raise SystemExit("pyserial is required: python3 -m pip install pyserial") from exc

    ser = serial.Serial(port=port, baudrate=baud, timeout=0.2, dsrdtr=False, rtscts=False)
    ser.dtr = False
    ser.rts = False
    return ser


def numbered_output_path(output, sequence):
    base = Path(output)
    if base.suffix.lower() == ".png":
        directory = base.parent if str(base.parent) else Path(".")
        stem = base.stem
    else:
        directory = base
        stem = "watchy-screenshot"

    directory.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    return directory / f"{stem}-{timestamp}-{sequence:03d}.png"


def main():
    default_output = "watchy-screenshot.png"
    parser = argparse.ArgumentParser(description="Capture a CityWeather Watchy screenshot over Serial.")
    parser.add_argument("port", nargs="?", help="Serial port, for example /dev/cu.usbserial-58910059051")
    parser.add_argument("output", nargs="?", default=default_output, help="Output PNG path")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=int, default=60)
    parser.add_argument("--scale", type=int, default=1, help="PNG scale factor")
    parser.add_argument("--once", action="store_true", help="Capture one screenshot and exit")
    parser.add_argument("--stdin", action="store_true", help="Read copied Serial output from stdin")
    args = parser.parse_args()

    if args.scale < 1:
        parser.error("--scale must be at least 1")

    output = args.output
    if args.stdin and args.port and args.output == default_output:
        output = args.port

    if args.stdin:
        width, height, packed = parse_screenshot_lines(sys.stdin)
        write_png(output, width, height, packed, args.scale)
        print(f"Wrote {output} ({width}x{height}, scale {args.scale})")
        return

    if not args.port:
        parser.error("port is required unless --stdin is used")

    with open_serial(args.port, args.baud) as ser:
        print("Waiting for screenshots. Hold the Watchy Menu button. Press Ctrl+C to stop.", file=sys.stderr)
        sequence = 1
        while True:
            width, height, packed = parse_screenshot_from_serial(
                ser,
                args.timeout if args.once else None,
            )
            if args.once:
                path = Path(output)
            else:
                path = numbered_output_path(output, sequence)
                sequence += 1
            write_png(path, width, height, packed, args.scale)
            print(f"Wrote {path} ({width}x{height}, scale {args.scale})")
            if args.once:
                return


if __name__ == "__main__":
    main()
