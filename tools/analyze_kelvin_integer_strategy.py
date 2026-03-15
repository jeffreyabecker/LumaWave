#!/usr/bin/env python3

import argparse
import math
import struct
from dataclasses import dataclass


MIN_KELVIN = 1200
MAX_KELVIN = 65000
RGB8_MAX = 255
RGB16_MAX = 65535
INT64_MIN = -(1 << 63)
INT64_MAX = (1 << 63) - 1
INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1
DEFAULT_Q32_BITS = (8, 10, 12)
DEFAULT_Q64_BITS = (16, 20, 24, 28)


@dataclass(frozen=True)
class FixedPointSpec:
    name: str
    q_bits: int
    int_bits: int
    min_int: int
    max_int: int


def build_strategy_specs(q32_bits: tuple[int, ...], q64_bits: tuple[int, ...]) -> list[FixedPointSpec]:
    specs: list[FixedPointSpec] = []

    for q_bits in q32_bits:
        specs.append(FixedPointSpec(name=f"int32-q{q_bits}", q_bits=q_bits, int_bits=32, min_int=INT32_MIN, max_int=INT32_MAX))

    for q_bits in q64_bits:
        specs.append(FixedPointSpec(name=f"int64-q{q_bits}", q_bits=q_bits, int_bits=64, min_int=INT64_MIN, max_int=INT64_MAX))

    return specs


def parse_q_bits(values: list[int], int_bits: int) -> tuple[int, ...]:
    if not values:
        return ()

    unique_values = sorted(set(values))
    for q_bits in unique_values:
        if q_bits <= 0:
            raise ValueError(f"Q bits must be positive for {int_bits}-bit analysis")
        if q_bits >= int_bits:
            raise ValueError(f"Q bits must be less than {int_bits} for signed {int_bits}-bit analysis")

    return tuple(unique_values)


def f32(value: float) -> float:
    return struct.unpack("!f", struct.pack("!f", float(value)))[0]


def cpp_round(value: float) -> int:
    if value >= 0.0:
        return math.floor(value + 0.5)
    return math.ceil(value - 0.5)


def clamp_u8(value: int) -> int:
    return max(0, min(RGB8_MAX, value))


def get_rgb_max(rgb_bits: int) -> int:
    if rgb_bits == 8:
        return RGB8_MAX
    if rgb_bits == 16:
        return RGB16_MAX
    raise ValueError("Only RGB8 and RGB16 outputs are supported")


def scale_byte_to_component(value: int, rgb_bits: int) -> int:
    rgb_max = get_rgb_max(rgb_bits)
    if rgb_max == RGB8_MAX:
        return value
    return ((value * rgb_max) + (RGB8_MAX // 2)) // RGB8_MAX


def clamp_kelvin(kelvin: int) -> int:
    return max(MIN_KELVIN, min(MAX_KELVIN, kelvin))


def kelvin_to_rgb_exact(kelvin: int) -> tuple[int, int, int]:
    temp = f32(clamp_kelvin(kelvin) / 100.0)

    red = 0
    green = 0
    blue = 0

    if temp <= 66.0:
        red = 255
        green = cpp_round(f32(f32(99.4708025861) * f32(math.log(temp))) - f32(161.1195681661))

        if temp <= 19.0:
            blue = 0
        else:
            blue = cpp_round(f32(f32(138.5177312231) * f32(math.log(f32(temp - 10.0)))) - f32(305.0447927307))
    else:
        red = cpp_round(f32(329.698727446) * f32(math.pow(f32(temp - 60.0), f32(-0.1332047592))))
        green = cpp_round(f32(288.1221695283) * f32(math.pow(f32(temp - 60.0), f32(-0.0755148492))))
        blue = 255

    return clamp_u8(red), clamp_u8(green), clamp_u8(blue)


def convert_output_rgb(rgb: tuple[int, int, int], rgb_bits: int) -> tuple[int, int, int]:
    return tuple(scale_byte_to_component(channel, rgb_bits) for channel in rgb)


@dataclass
class IntegerTracker:
    max_abs_value: int = 0
    overflow_count: int = 0
    min_int: int = INT64_MIN
    max_int: int = INT64_MAX

    def check(self, value: int) -> int:
        abs_value = abs(value)
        if abs_value > self.max_abs_value:
            self.max_abs_value = abs_value
        if value < self.min_int or value > self.max_int:
            self.overflow_count += 1
        return value


class FixedPointKelvinStrategy:
    def __init__(self, spec: FixedPointSpec) -> None:
        self.spec = spec
        self.q_bits = spec.q_bits
        self.q_one = 1 << self.q_bits
        self.tracker = IntegerTracker(min_int=spec.min_int, max_int=spec.max_int)

        self.ln2_q = round(math.log(2.0) * self.q_one)
        self.red_scale_q = round(329.698727446 * self.q_one)
        self.red_exp_q = round(-0.1332047592 * self.q_one)
        self.green_scale_q = round(288.1221695283 * self.q_one)
        self.green_exp_q = round(-0.0755148492 * self.q_one)
        self.low_green_log_scale_q = round(99.4708025861 * self.q_one)
        self.low_green_log_offset_q = round(161.1195681661 * self.q_one)
        self.low_blue_log_scale_q = round(138.5177312231 * self.q_one)
        self.low_blue_log_offset_q = round(305.0447927307 * self.q_one)
        self.temp_10_q = 10 * self.q_one
        self.temp_19_q = 19 * self.q_one
        self.temp_60_q = 60 * self.q_one
        self.temp_66_q = 66 * self.q_one

    def checked(self, value: int) -> int:
        return self.tracker.check(value)

    def round_div(self, numerator: int, denominator: int) -> int:
        self.checked(numerator)
        self.checked(denominator)

        if denominator == 0:
            raise ZeroDivisionError("division by zero")

        if denominator < 0:
            numerator = -numerator
            denominator = -denominator

        if numerator >= 0:
            return self.checked((numerator + (denominator // 2)) // denominator)

        return self.checked(-(((-numerator) + (denominator // 2)) // denominator))

    def mul_q(self, left: int, right: int) -> int:
        return self.round_div(self.checked(left * right), self.q_one)

    def div_q(self, left: int, right: int) -> int:
        return self.round_div(self.checked(left * self.q_one), right)

    def q_to_int(self, value_q: int) -> int:
        return self.round_div(value_q, self.q_one)

    def ln_q(self, value_q: int) -> int:
        if value_q <= 0:
            raise ValueError("ln_q requires a positive value")

        normalized = value_q
        exponent = 0

        while normalized < self.q_one:
            normalized = self.checked(normalized << 1)
            exponent -= 1

        while normalized >= (2 * self.q_one):
            normalized >>= 1
            exponent += 1

        y = self.div_q(normalized - self.q_one, normalized + self.q_one)
        y_sq = self.mul_q(y, y)

        term = y
        series = term

        for denominator in (3, 5, 7, 9, 11, 13):
            term = self.mul_q(term, y_sq)
            series = self.checked(series + self.round_div(term, denominator))

        return self.checked((series << 1) + (exponent * self.ln2_q))

    def exp_q(self, value_q: int) -> int:
        shift = self.round_div(value_q, self.ln2_q)
        reduced = self.checked(value_q - (shift * self.ln2_q))

        term = self.q_one
        series = self.q_one

        for divisor in range(1, 9):
            term = self.round_div(self.checked(term * reduced), self.checked(self.q_one * divisor))
            series = self.checked(series + term)

        if shift >= 0:
            return self.checked(series << shift)

        return self.round_div(series, 1 << (-shift))

    def pow_fraction_q(self, base_q: int, exponent_q: int) -> int:
        return self.exp_q(self.mul_q(self.ln_q(base_q), exponent_q))

    def convert(self, kelvin: int) -> tuple[int, int, int]:
        clamped_kelvin = clamp_kelvin(kelvin)
        temp_q = self.round_div(self.checked(clamped_kelvin * self.q_one), 100)

        if temp_q <= self.temp_66_q:
            red = 255
            green_q = self.checked(self.mul_q(self.low_green_log_scale_q, self.ln_q(temp_q)) - self.low_green_log_offset_q)
            green = self.q_to_int(green_q)

            if temp_q <= self.temp_19_q:
                blue = 0
            else:
                blue_q = self.checked(
                    self.mul_q(self.low_blue_log_scale_q, self.ln_q(temp_q - self.temp_10_q)) - self.low_blue_log_offset_q
                )
                blue = self.q_to_int(blue_q)
        else:
            delta_q = temp_q - self.temp_60_q
            red_q = self.mul_q(self.red_scale_q, self.pow_fraction_q(delta_q, self.red_exp_q))
            green_q = self.mul_q(self.green_scale_q, self.pow_fraction_q(delta_q, self.green_exp_q))
            red = self.q_to_int(red_q)
            green = self.q_to_int(green_q)
            blue = 255

        return clamp_u8(red), clamp_u8(green), clamp_u8(blue)


@dataclass
class ErrorStats:
    avg_abs: float = 0.0
    rms: float = 0.0
    max_abs: float = 0.0


@dataclass
class SampleRow:
    kelvin: int
    exact: tuple[int, int, int]
    candidate: tuple[int, int, int]
    channel_errors: tuple[float, float, float]
    avg_error: float
    max_error: float


@dataclass
class StrategyRun:
    spec: FixedPointSpec
    rgb_bits: int
    rows: list[SampleRow]
    mismatch_count: int
    tracker: IntegerTracker


def compute_channel_stats(rows: list[SampleRow], channel_index: int) -> ErrorStats:
    total_abs = 0.0
    total_sq = 0.0
    max_abs = 0.0

    for row in rows:
        error = row.channel_errors[channel_index]
        total_abs += error
        total_sq += error * error
        max_abs = max(max_abs, error)

    sample_count = max(1, len(rows))
    return ErrorStats(
        avg_abs=total_abs / sample_count,
        rms=math.sqrt(total_sq / sample_count),
        max_abs=max_abs,
    )


def compute_overall_stats(rows: list[SampleRow]) -> ErrorStats:
    total_abs = 0.0
    total_sq = 0.0
    max_abs = 0.0
    count = 0

    for row in rows:
        for error in row.channel_errors:
            total_abs += error
            total_sq += error * error
            max_abs = max(max_abs, error)
            count += 1

    count = max(1, count)
    return ErrorStats(
        avg_abs=total_abs / count,
        rms=math.sqrt(total_sq / count),
        max_abs=max_abs,
    )


def analyze_range(strategy: FixedPointKelvinStrategy, start_kelvin: int, stop_kelvin: int, step_kelvin: int, rgb_bits: int) -> list[SampleRow]:
    rows: list[SampleRow] = []
    kelvin = start_kelvin
    rgb_max = get_rgb_max(rgb_bits)

    while kelvin <= stop_kelvin:
        exact = convert_output_rgb(kelvin_to_rgb_exact(kelvin), rgb_bits)
        candidate = convert_output_rgb(strategy.convert(kelvin), rgb_bits)
        channel_errors = tuple(abs(candidate[index] - exact[index]) / rgb_max * 100.0 for index in range(3))
        avg_error = sum(channel_errors) / 3.0
        max_error = max(channel_errors)
        rows.append(
            SampleRow(
                kelvin=kelvin,
                exact=exact,
                candidate=candidate,
                channel_errors=channel_errors,
                avg_error=avg_error,
                max_error=max_error,
            )
        )
        kelvin += step_kelvin

    if rows[-1].kelvin != stop_kelvin:
        exact = convert_output_rgb(kelvin_to_rgb_exact(stop_kelvin), rgb_bits)
        candidate = convert_output_rgb(strategy.convert(stop_kelvin), rgb_bits)
        channel_errors = tuple(abs(candidate[index] - exact[index]) / rgb_max * 100.0 for index in range(3))
        rows.append(
            SampleRow(
                kelvin=stop_kelvin,
                exact=exact,
                candidate=candidate,
                channel_errors=channel_errors,
                avg_error=sum(channel_errors) / 3.0,
                max_error=max(channel_errors),
            )
        )

    return rows


def run_strategy(spec: FixedPointSpec, start_kelvin: int, stop_kelvin: int, step_kelvin: int, rgb_bits: int) -> StrategyRun:
    strategy = FixedPointKelvinStrategy(spec)
    rows = analyze_range(strategy, start_kelvin, stop_kelvin, step_kelvin, rgb_bits)
    mismatch_count = sum(1 for row in rows if row.max_error > 0.0)
    return StrategyRun(spec=spec, rgb_bits=rgb_bits, rows=rows, mismatch_count=mismatch_count, tracker=strategy.tracker)


def print_summary(rows: list[SampleRow]) -> None:
    print("Summary error table vs KelvinToRgbExactStrategy")
    print("channel | avg abs error % | rms error % | max abs error %")
    print("------- | --------------- | ----------- | ---------------")

    for name, index in (("red", 0), ("green", 1), ("blue", 2)):
        stats = compute_channel_stats(rows, index)
        print(f"{name:7} | {stats.avg_abs:15.6f} | {stats.rms:11.6f} | {stats.max_abs:15.6f}")

    overall = compute_overall_stats(rows)
    print(f"overall | {overall.avg_abs:15.6f} | {overall.rms:11.6f} | {overall.max_abs:15.6f}")


def compute_average_sample_error(rows: list[SampleRow]) -> float:
    if not rows:
        return 0.0
    return sum(row.avg_error for row in rows) / len(rows)


def compute_max_sample_error(rows: list[SampleRow]) -> float:
    if not rows:
        return 0.0
    return max(row.max_error for row in rows)


def print_strategy_table(runs: list[StrategyRun]) -> None:
    print("Strategy comparison vs KelvinToRgbExactStrategy")
    print(
        "strategy | q bits | int bits | sample mismatches | avg channel err % | max channel err % | avg sample err % | max sample err %"
    )
    print(
        "-------- | ------ | -------- | ----------------- | ----------------- | ----------------- | ---------------- | ----------------"
    )

    for run in runs:
        overall = compute_overall_stats(run.rows)
        avg_sample_error = compute_average_sample_error(run.rows)
        max_sample_error = compute_max_sample_error(run.rows)
        print(
            f"{run.spec.name:8} | "
            f"{run.spec.q_bits:6} | "
            f"{run.spec.int_bits:8} | "
            f"{run.mismatch_count:17} | "
            f"{overall.avg_abs:17.6f} | "
            f"{overall.max_abs:17.6f} | "
            f"{avg_sample_error:16.6f} | "
            f"{max_sample_error:16.6f}"
        )


def print_viability_table(runs: list[StrategyRun]) -> None:
    print("\nInteger range viability")
    print("strategy | q bits | max intermediate magnitude | overflow events")
    print("-------- | ------ | -------------------------- | ---------------")
    for run in runs:
        print(
            f"{run.spec.name:8} | {run.spec.q_bits:6} | {run.tracker.max_abs_value:26} | {run.tracker.overflow_count:15}"
        )


def print_rows(title: str, rows: list[SampleRow]) -> None:
    if not rows:
        print(f"\n{title}")
        print("No rows to display.")
        return

    print(f"\n{title}")
    print("kelvin | exact rgb | candidate rgb | red err % | green err % | blue err % | avg err % | max err %")
    print("------ | --------- | ------------- | --------- | ----------- | ---------- | --------- | ---------")
    for row in rows:
        print(
            f"{row.kelvin:6} | "
            f"{row.exact!s:9} | "
            f"{row.candidate!s:13} | "
            f"{row.channel_errors[0]:9.6f} | "
            f"{row.channel_errors[1]:11.6f} | "
            f"{row.channel_errors[2]:10.6f} | "
            f"{row.avg_error:9.6f} | "
            f"{row.max_error:9.6f}"
        )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compare KelvinToRgbExactStrategy against proposed fixed-point integer-only candidates "
            "for both signed 32-bit and signed 64-bit intermediate ranges."
        )
    )
    parser.add_argument("--start", type=int, default=MIN_KELVIN, help="Starting Kelvin value.")
    parser.add_argument("--stop", type=int, default=MAX_KELVIN, help="Ending Kelvin value.")
    parser.add_argument("--step", type=int, default=500, help="Kelvin increment between samples.")
    parser.add_argument("--top", type=int, default=20, help="Number of worst samples to print.")
    parser.add_argument("--rgb-bits", type=int, choices=(8, 16), default=8, help="Output component width to compare.")
    parser.add_argument(
        "--q32",
        type=int,
        nargs="*",
        default=list(DEFAULT_Q32_BITS),
        help="Q formats to evaluate for signed 32-bit intermediates.",
    )
    parser.add_argument(
        "--q64",
        type=int,
        nargs="*",
        default=list(DEFAULT_Q64_BITS),
        help="Q formats to evaluate for signed 64-bit intermediates.",
    )
    parser.add_argument("--details", action="store_true", help="Print the full sampled error table.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if args.step <= 0:
        raise SystemExit("--step must be positive")

    start_kelvin = clamp_kelvin(args.start)
    stop_kelvin = clamp_kelvin(args.stop)

    if start_kelvin > stop_kelvin:
        raise SystemExit("--start must be less than or equal to --stop")

    try:
        rgb_max = get_rgb_max(args.rgb_bits)
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc

    try:
        q32_bits = parse_q_bits(args.q32, 32)
        q64_bits = parse_q_bits(args.q64, 64)
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc

    if not q32_bits and not q64_bits:
        raise SystemExit("Provide at least one Q format via --q32 or --q64")

    strategy_specs = build_strategy_specs(q32_bits, q64_bits)
    runs = [run_strategy(spec, start_kelvin, stop_kelvin, args.step, args.rgb_bits) for spec in strategy_specs]
    sample_count = len(runs[0].rows) if runs else 0

    print(
        f"Samples: {sample_count} from {start_kelvin}K to {stop_kelvin}K in {args.step}K increments "
        f"plus terminal endpoint coverage."
    )
    print(f"RGB domain: 0..{rgb_max} (RGB{args.rgb_bits}).")
    print(f"Q32 sweep: {', '.join(str(value) for value in q32_bits) if q32_bits else 'none'}")
    print(f"Q64 sweep: {', '.join(str(value) for value in q64_bits) if q64_bits else 'none'}")
    print_strategy_table(runs)
    print_viability_table(runs)

    for run in runs:
        mismatched_rows = [row for row in run.rows if row.max_error > 0.0]
        worst_rows = sorted(mismatched_rows, key=lambda row: (row.max_error, row.avg_error, row.kelvin), reverse=True)[: args.top]
        print(f"\nPer-channel summary for {run.spec.name}")
        print_summary(run.rows)
        print_rows(f"Worst sampled Kelvin points by max channel error for {run.spec.name}", worst_rows)

    if args.details:
        for run in runs:
            print_rows(f"Full sampled error table for {run.spec.name}", run.rows)


if __name__ == "__main__":
    main()