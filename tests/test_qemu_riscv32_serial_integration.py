from __future__ import annotations

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_DIR = ROOT / "platform" / "qemu-riscv32"
DEFAULT_EXAMPLE = ROOT / "examples" / "qemu_riscv_fb_demo.rz"
MEMORY_REPORT_EXAMPLE = ROOT / "examples" / "qemu_riscv_memory_report_demo.rz"
MULTISTATEMENT_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_multistatement_source_demo.rz"
TEMPS_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_temps_method_demo.rz"
PACKAGE_COMMENT_ROUNDTRIP_EXAMPLE = ROOT / "examples" / "qemu_riscv_package_comment_roundtrip_demo.rz"
BINARY_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_binary_method_demo.rz"
CONDITIONAL_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_conditional_method_demo.rz"
PAREN_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_parenthesized_method_demo.rz"
MULTIKEYWORD_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_multikeyword_method_demo.rz"
EXPRESSION_KEYWORD_SEND_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_expression_keyword_send_demo.rz"
NIL_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_nil_method_demo.rz"
SELF_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_self_method_demo.rz"
SMALL_INTEGER_LITERAL_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_small_integer_literal_demo.rz"
STRING_LITERAL_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_string_literal_demo.rz"
BOOLEAN_LITERAL_METHOD_SOURCE_EXAMPLE = ROOT / "examples" / "qemu_riscv_in_image_boolean_literal_demo.rz"


def _build_elf(build_dir: Path, example_path: Path = DEFAULT_EXAMPLE, *, profile: str = "dev") -> Path:
    result = subprocess.run(
        [
            "make",
            "-C",
            str(PLATFORM_DIR),
            f"BUILD_DIR={build_dir}",
            f"RV32_PROFILE={profile}",
            f"EXAMPLE={example_path}",
            "clean",
            "all",
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise AssertionError(
            "RV32 QEMU build failed\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )
    return build_dir / "recorz-qemu-riscv32-mvp.elf"


@unittest.skipUnless(
    shutil.which("qemu-system-riscv32") and shutil.which("riscv64-unknown-elf-gcc"),
    "QEMU RV32 serial integration test requires qemu-system-riscv32 and riscv64-unknown-elf-gcc",
)
class QemuRiscv32SerialIntegrationTests(unittest.TestCase):
    def test_default_demo_boots_and_prints_transcript_over_serial(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-serial-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: booting", output)
            self.assertIn("HELLO FROM RECORZ", output)
            self.assertIn("SIZE: 1024 x 768", output)
            self.assertIn("RAMFB ONLINE.", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_memory_report_demo_prints_usage_within_budget(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-memory-report-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, MEMORY_REPORT_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("MEMORY", output)
            self.assertIn("PROFILE DEV", output)

            expected_limits = {
                "HEAP": 768,
                "DCLS": 24,
                "NOBJ": 24,
                "MSRC": 64,
                "MSRP": 16384,
                "RSTR": 8192,
                "SSTR": 16384,
                "SNAP": 65536,
                "MONO": 16,
            }
            for label, expected_limit in expected_limits.items():
                match = re.search(rf"{label} (\d+)/(\d+)", output)
                self.assertIsNotNone(match, f"missing {label} line in output:\n{output}")
                used = int(match.group(1))
                limit = int(match.group(2))
                self.assertEqual(limit, expected_limit, label)
                self.assertLess(used, limit, label)

    def test_runtime_string_pool_compacts_dead_workspace_strings(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-runtime-strings-") as temp_dir:
            temp_path = Path(temp_dir)
            example_path = temp_path / "runtime_string_compaction.rz"
            class_source = (
                "RecorzKernelClass: #ReplayLongRuntimeStringReuse superclass: #Object instanceVariableNames: ''\n!\n"
                "value\n    ^self\n!\n"
                "setValue: extremelyVerboseArgumentNameForSourceInflationPurposes\n    ^self\n!\n"
                "RecorzKernelClassSide: #ReplayLongRuntimeStringReuse\n!\n"
                "detail\n    ^self"
            )
            escaped_class_source = class_source.replace("'", "''")
            repeated_updates = "\n".join(
                "Workspace setContents: (KernelInstaller fileOutClassNamed: 'ReplayLongRuntimeStringReuse')."
                for _ in range(70)
            )
            example_path.write_text(
                "\n".join(
                    [
                        "Display clear.",
                        f"KernelInstaller fileInClassChunks: '{escaped_class_source}'.",
                        repeated_updates,
                        "Transcript show: Workspace contents.",
                    ]
                ),
                encoding="utf-8",
            )
            elf_path = _build_elf(temp_path / "build", example_path)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("RecorzKernelClass:", output)
            self.assertIn("ReplayLongRuntime", output)
            self.assertIn("setValue:", output)
            self.assertNotIn("panic: runtime string pool overflow", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_multistatement_methods_and_unary_expression_chains(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-multistatement-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, MULTISTATEMENT_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class Reporter", output)
            self.assertIn("1024", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_temporaries_and_local_assignment(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-temps-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, TEMPS_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class TempReporter", output)
            self.assertIn("TEMP", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_binary_method_expressions(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-binary-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, BINARY_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class BinaryReporter", output)
            self.assertIn("BINARY", output)
            self.assertIn("6", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_comparisons_and_conditional_control_flow(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-conditional-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, CONDITIONAL_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class ConditionalReporter", output)
            self.assertIn("COND", output)
            self.assertIn("LT", output)
            self.assertIn("GE", output)
            self.assertIn("EQ", output)
            self.assertIn("IFFALSE", output)
            self.assertIn("GT", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_parenthesized_method_expressions(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-parenthesized-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, PAREN_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class ParenReporter", output)
            self.assertIn("PAREN", output)
            self.assertIn("6", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_multi_keyword_method_headers(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-multikeyword-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, MULTIKEYWORD_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class KeywordReporter", output)
            self.assertIn("KEYWORD", output)
            self.assertIn("6", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_keyword_sends_inside_method_expressions(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-expression-keyword-send-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, EXPRESSION_KEYWORD_SEND_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class KeywordExpressionReporter", output)
            self.assertIn("EXPR", output)
            self.assertIn("RETURN", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_nil_in_live_method_bodies(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-nil-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, NIL_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class NilReporter", output)
            self.assertIn("NIL", output)
            self.assertIn("KEPT", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_self_in_live_method_bodies(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-self-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, SELF_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class SelfReporter", output)
            self.assertIn("SELF", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_small_integer_literals_in_live_method_bodies(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-small-integer-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, SMALL_INTEGER_LITERAL_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class LiteralReporter", output)
            self.assertIn("INT", output)
            self.assertIn("6", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_string_literals_in_live_method_bodies(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-string-literal-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, STRING_LITERAL_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class StringReporter", output)
            self.assertIn("STR", output)
            self.assertIn("OK", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_in_image_source_compiler_supports_true_and_false_literals_in_live_method_bodies(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-boolean-literal-method-source-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, BOOLEAN_LITERAL_METHOD_SOURCE_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            self.assertIn("recorz qemu-riscv32 mvp: created class BooleanReporter", output)
            self.assertIn("OBJECT: TRUTHVALUE", output)
            self.assertIn("CLASS: TRUE", output)
            self.assertIn("OBJECT: FALSEVALUE", output)
            self.assertIn("CLASS: FALSE", output)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)

    def test_package_comment_round_trips_through_file_out_source(self) -> None:
        with tempfile.TemporaryDirectory(prefix="qemu-riscv32-package-comment-") as temp_dir:
            build_dir = Path(temp_dir)
            elf_path = _build_elf(build_dir, PACKAGE_COMMENT_ROUNDTRIP_EXAMPLE)
            process = subprocess.Popen(
                [
                    "qemu-system-riscv32",
                    "-machine",
                    "virt",
                    "-m",
                    "32M",
                    "-smp",
                    "1",
                    "-kernel",
                    str(elf_path),
                    "-serial",
                    "stdio",
                    "-display",
                    "none",
                    "-device",
                    "ramfb",
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                try:
                    output, _ = process.communicate(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    output, _ = process.communicate(timeout=5.0)
            finally:
                if process.stdout is not None:
                    process.stdout.close()

            output = output.replace("\r", "")
            output_no_newlines = output.replace("\n", "")
            self.assertIn("RecorzKernelPackage:", output_no_newlines)
            self.assertIn("Tools", output_no_newlines)
            self.assertIn("comment:", output_no_newlines)
            self.assertIn("Shared utilities", output_no_newlines)
            self.assertIn("PackageRoundtrip", output_no_newlines)
            self.assertIn("recorz qemu-riscv32 mvp: rendered", output)


if __name__ == "__main__":
    unittest.main()
