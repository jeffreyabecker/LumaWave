#!/usr/bin/env python3

import argparse
import math
import struct
from dataclasses import dataclass


RGB8_MAX = 255
RGB16_MAX = 65535
Q14_ONE = 1 << 14


def f32(value: float) -> float:
    return struct.unpack("!f", struct.pack("!f", float(value)))[0]


def clamp01_float(value: float) -> float:
    return f32(min(1.0, max(0.0, value)))


def quantize_unit(value: float, scale: int) -> int:
    clamped = min(1.0, max(0.0, value))
    return int(round(clamped * scale))


def dequantize_unit(value: int, scale: int) -> float:
    return value / float(scale)


def rgb_unit_to_rgb(rgb_unit: tuple[float, float, float], rgb_max: int) -> tuple[int, int, int]:
    return tuple(quantize_unit(channel, rgb_max) for channel in rgb_unit)


def float_hsb_to_rgb(h: float, s: float, brightness: float, rgb_max: int) -> tuple[int, int, int]:
    h = clamp01_float(h)
    s = clamp01_float(s)
    brightness = clamp01_float(brightness)

    if s == 0.0:
        return rgb_unit_to_rgb((brightness, brightness, brightness), rgb_max)

    if h < 0.0:
        h = f32(h + 1.0)
    elif h >= 1.0:
        h = f32(h - 1.0)

    h6 = f32(h * 6.0)
    sector = int(h6)
    frac = f32(h6 - sector)
    q = f32(brightness * f32(1.0 - f32(s * frac)))
    p = f32(brightness * f32(1.0 - s))
    t = f32(brightness * f32(1.0 - f32(s * f32(1.0 - frac))))

    if sector == 0:
        rgb = (brightness, t, p)
    elif sector == 1:
        rgb = (q, brightness, p)
    elif sector == 2:
        rgb = (p, brightness, t)
    elif sector == 3:
        rgb = (p, q, brightness)
    elif sector == 4:
        rgb = (t, p, brightness)
    else:
        rgb = (brightness, p, q)

    return rgb_unit_to_rgb(rgb, rgb_max)


def calc_hsl_component_float(p: float, q: float, t: float) -> float:
    if t < 0.0:
        t = f32(t + 1.0)
    if t > 1.0:
        t = f32(t - 1.0)
    if t < (1.0 / 6.0):
        return f32(p + f32(f32(q - p) * f32(6.0 * t)))
    if t < 0.5:
        return q
    if t < (2.0 / 3.0):
        return f32(p + f32(f32(q - p) * f32(f32((2.0 / 3.0) - t) * 6.0)))
    return p


def float_hsl_to_rgb(h: float, s: float, lightness: float, rgb_max: int) -> tuple[int, int, int]:
    h = clamp01_float(h)
    s = clamp01_float(s)
    lightness = clamp01_float(lightness)

    if s == 0.0 or lightness == 0.0:
        return rgb_unit_to_rgb((lightness, lightness, lightness), rgb_max)

    if lightness < 0.5:
        q = f32(lightness * f32(1.0 + s))
    else:
        q = f32(f32(lightness + s) - f32(lightness * s))
    p = f32(f32(2.0 * lightness) - q)
    r = calc_hsl_component_float(p, q, f32(h + (1.0 / 3.0)))
    g = calc_hsl_component_float(p, q, h)
    b = calc_hsl_component_float(p, q, f32(h - (1.0 / 3.0)))
    return rgb_unit_to_rgb((r, g, b), rgb_max)


def int_mul_div(a: int, b: int, scale: int) -> int:
    return (a * b + (scale // 2)) // scale


def int_hsb_to_rgb(h: int, s: int, brightness: int, scale: int, rgb_max: int) -> tuple[int, int, int]:
    h = max(0, min(scale, h))
    s = max(0, min(scale, s))
    brightness = max(0, min(scale, brightness))

    if s == 0:
        value = int_mul_div(brightness, rgb_max, scale)
        return value, value, value

    h6 = h * 6
    sector = min(5, h6 // scale)
    frac = h6 - sector * scale

    p = int_mul_div(brightness, scale - s, scale)
    q = int_mul_div(brightness, scale - int_mul_div(s, frac, scale), scale)
    t = int_mul_div(brightness, scale - int_mul_div(s, scale - frac, scale), scale)

    if sector == 0:
        rgb = (brightness, t, p)
    elif sector == 1:
        rgb = (q, brightness, p)
    elif sector == 2:
        rgb = (p, brightness, t)
    elif sector == 3:
        rgb = (p, q, brightness)
    elif sector == 4:
        rgb = (t, p, brightness)
    else:
        rgb = (brightness, p, q)

    return tuple(int_mul_div(channel, rgb_max, scale) for channel in rgb)


def int_calc_hsl_component(p: int, q: int, t: int, scale: int) -> int:
    if t < 0:
        t += scale
    if t > scale:
        t -= scale

    one_sixth = scale // 6
    two_thirds = (2 * scale) // 3

    if t < one_sixth:
        return p + int_mul_div(q - p, 6 * t, scale)
    if t < (scale // 2):
        return q
    if t < two_thirds:
        return p + int_mul_div(q - p, 6 * (two_thirds - t), scale)
    return p


def int_hsl_to_rgb(h: int, s: int, lightness: int, scale: int, rgb_max: int) -> tuple[int, int, int]:
    h = max(0, min(scale, h))
    s = max(0, min(scale, s))
    lightness = max(0, min(scale, lightness))

    if s == 0 or lightness == 0:
        value = int_mul_div(lightness, rgb_max, scale)
        return value, value, value

    if lightness < (scale // 2):
        q = int_mul_div(lightness, scale + s, scale)
    else:
        q = lightness + s - int_mul_div(lightness, s, scale)

    p = 2 * lightness - q
    one_third = scale // 3
    r = int_calc_hsl_component(p, q, h + one_third, scale)
    g = int_calc_hsl_component(p, q, h, scale)
    b = int_calc_hsl_component(p, q, h - one_third, scale)
    return tuple(int_mul_div(channel, rgb_max, scale) for channel in (r, g, b))


@dataclass
class ErrorStats:
    avg_abs: float = 0.0
    rms: float = 0.0
    max_abs: float = 0.0


def compute_error_stats(
    reference: list[tuple[int, int, int]],
    candidate: list[tuple[int, int, int]],
    rgb_max: int,
) -> ErrorStats:
    total_abs = 0.0
    total_sq = 0.0
    max_abs = 0.0
    count = 0

    for ref_rgb, cand_rgb in zip(reference, candidate):
        for ref_channel, cand_channel in zip(ref_rgb, cand_rgb):
            diff = abs(cand_channel - ref_channel) / rgb_max * 100.0
            total_abs += diff
            total_sq += diff * diff
            max_abs = max(max_abs, diff)
            count += 1

    return ErrorStats(
        avg_abs=total_abs / count,
        rms=math.sqrt(total_sq / count),
        max_abs=max_abs,
    )


def generate_hue_samples(sample_count: int) -> list[float]:
    if sample_count <= 1:
        return [0.0]
    return [index / float(sample_count - 1) for index in range(sample_count)]


def analyze_space(sample_count: int, space: str, rgb_max: int) -> dict[str, ErrorStats]:
    hues = generate_hue_samples(sample_count)

    if space == "hsl":
        reference = [float_hsl_to_rgb(hue, 1.0, 0.5, rgb_max) for hue in hues]
        rgb8 = [int_hsl_to_rgb(quantize_unit(hue, 255), 255, 128, 255, rgb_max) for hue in hues]
        rgb16 = [int_hsl_to_rgb(quantize_unit(hue, 65535), 65535, 32768, 65535, rgb_max) for hue in hues]
        q14 = [int_hsl_to_rgb(quantize_unit(hue, Q14_ONE), Q14_ONE, Q14_ONE // 2, Q14_ONE, rgb_max) for hue in hues]
    else:
        reference = [float_hsb_to_rgb(hue, 1.0, 1.0, rgb_max) for hue in hues]
        rgb8 = [int_hsb_to_rgb(quantize_unit(hue, 255), 255, 255, 255, rgb_max) for hue in hues]
        rgb16 = [int_hsb_to_rgb(quantize_unit(hue, 65535), 65535, 65535, 65535, rgb_max) for hue in hues]
        q14 = [int_hsb_to_rgb(quantize_unit(hue, Q14_ONE), Q14_ONE, Q14_ONE, Q14_ONE, rgb_max) for hue in hues]

    return {
        "8-bit": compute_error_stats(reference, rgb8, rgb_max),
        "16-bit": compute_error_stats(reference, rgb16, rgb_max),
        "Q1.14": compute_error_stats(reference, q14, rgb_max),
    }


def print_table(space: str, rgb_bits: int, stats: dict[str, ErrorStats]) -> None:
    print(f"\n{space.upper()} rainbow error vs float32 RGB{rgb_bits} baseline")
    print("representation | avg abs error % | rms error % | max abs error %")
    print("-------------- | --------------- | ----------- | ---------------")
    for name, values in stats.items():
        print(f"{name:14} | {values.avg_abs:15.6f} | {values.rms:11.6f} | {values.max_abs:15.6f}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compare HSL/HSB rainbow conversion error for 8-bit, 16-bit, and Q1.14 math against "
            "a float32 baseline, reported for both RGB8 and RGB16 output conversions."
        )
    )
    parser.add_argument("--samples", type=int, default=1024, help="Number of hue samples across the rainbow.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    print(f"Samples: {args.samples}")
    print("Rainbow assumptions: H sweeps 0..1, S=1.0, L=0.5 for HSL, B=1.0 for HSB.")

    for rgb_bits, rgb_max in ((8, RGB8_MAX), (16, RGB16_MAX)):
        for space in ("hsl", "hsb"):
            print_table(space, rgb_bits, analyze_space(args.samples, space, rgb_max))


if __name__ == "__main__":
    main()