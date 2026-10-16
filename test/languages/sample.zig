// Author: Wirot Chookeaw (chin6700x)
// Copyright: (c) 2026 Wirot Chookeaw.
// License: Apache License 2.0
//
// ⚡ nocker: Multi-Language Parity Benchmark - Zig Engine

const std = @import("std");

const MathError = error{
    DivisionByZero,
    Overflow,
};

pub fn fast_power(base: u64, exponent: u32) MathError!u64 {
    var result: u64 = 1;
    var b = base;
    var exp = exponent;

    while (exp > 0) : (exp >>= 1) {
        if ((exp & 1) == 1) {
            const ov = @mulWithOverflow(result, b);
            if (ov[1] != 0) return MathError.Overflow;
            result = ov[0];
        }
        if (exp > 1) {
            const ov_b = @mulWithOverflow(b, b);
            if (ov_b[1] != 0) return MathError.Overflow;
            b = ov_b[0];
        }
    }
    return result;
}

pub fn main() !void {
    const val = try fast_power(2, 16);
    std.debug.print("⚡ [Zig-Runner] 2^16 Fast Power = {d}\n", .{val});
}
