const std = @import("std");
/// Zig version. When writing code that supports multiple versions of Zig, prefer
/// feature detection (i.e. with `@hasDecl` or `@hasField`) over version checks.
pub const zig_version = std.SemanticVersion.parse(zig_version_string) catch unreachable;
pub const zig_version_string = "0.12.0-dev.1108+aeadcb391";
pub const zig_backend = std.builtin.CompilerBackend.stage2_llvm;

pub const output_mode = std.builtin.OutputMode.Exe;
pub const link_mode = std.builtin.LinkMode.Dynamic;
pub const is_test = true;
pub const single_threaded = false;
pub const abi = std.Target.Abi.gnu;
pub const cpu: std.Target.Cpu = .{
    .arch = .x86_64,
    .model = &std.Target.x86.cpu.sandybridge,
    .features = std.Target.x86.featureSet(&[_]std.Target.x86.Feature{
        .@"64bit",
        .aes,
        .avx,
        .cmov,
        .crc32,
        .cx16,
        .cx8,
        .false_deps_popcnt,
        .fast_15bytenop,
        .fast_scalar_fsqrt,
        .fast_shld_rotate,
        .fxsr,
        .idivq_to_divl,
        .macrofusion,
        .mmx,
        .no_bypass_delay_mov,
        .nopl,
        .pclmul,
        .popcnt,
        .sahf,
        .slow_3ops_lea,
        .slow_unaligned_mem_32,
        .sse,
        .sse2,
        .sse3,
        .sse4_1,
        .sse4_2,
        .ssse3,
        .vzeroupper,
        .x87,
        .xsave,
        .xsaveopt,
    }),
};
pub const os = std.Target.Os{
    .tag = .linux,
    .version_range = .{ .linux = .{
        .range = .{
            .min = .{
                .major = 6,
                .minor = 2,
                .patch = 0,
            },
            .max = .{
                .major = 6,
                .minor = 2,
                .patch = 0,
            },
        },
        .glibc = .{
            .major = 2,
            .minor = 19,
            .patch = 0,
        },
    }},
};
pub const target = std.Target{
    .cpu = cpu,
    .os = os,
    .abi = abi,
    .ofmt = object_format,
};
pub const object_format = std.Target.ObjectFormat.elf;
pub const mode = std.builtin.OptimizeMode.Debug;
pub const link_libc = true;
pub const link_libcpp = false;
pub const have_error_return_tracing = true;
pub const valgrind_support = true;
pub const sanitize_thread = false;
pub const position_independent_code = true;
pub const position_independent_executable = false;
pub const strip_debug_info = false;
pub const code_model = std.builtin.CodeModel.default;
pub const omit_frame_pointer = false;
pub var test_functions: []const std.builtin.TestFn = undefined; // overwritten later
pub const test_io_mode = .blocking;
