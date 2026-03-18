import tempfile
import unittest

from recorz.bootstrap import bootstrap_image
from recorz.compiler import compile_method
from recorz.image import load_image_manifest, save_image_manifest
from recorz.model import InvalidNonLocalReturn, MessageNotUnderstood, RecorzRuntimeError


class RuntimeTests(unittest.TestCase):
    def test_smalltalk_binary_precedence(self) -> None:
        _image, vm = bootstrap_image()
        self.assertEqual(vm.evaluate("3 + 4 * 5"), 35)

    def test_recursive_method_and_blocks(self) -> None:
        _image, vm = bootstrap_image()
        self.assertEqual(vm.evaluate("3 factorial"), 6)

    def test_do_it_temporaries_and_assignment(self) -> None:
        _image, vm = bootstrap_image()
        self.assertEqual(vm.evaluate("| width height pixels | width := 640. height := 480. pixels := width * height. pixels"), 307200)

    def test_lexical_capture_mutation(self) -> None:
        image, vm = bootstrap_image()
        receiver = vm.send(image.class_named("Object"), "new", [])
        self.assertEqual(vm.send(receiver, "captureExample", []), 3)

    def test_wrong_arity_raises_runtime_error(self) -> None:
        image, vm = bootstrap_image()
        image.install_method("Object", compile_method("echo: arg\n    ^arg", "Object", []))
        receiver = vm.send(image.class_named("Object"), "new", [])

        with self.assertRaisesRegex(RecorzRuntimeError, "expected 1 arguments but received 0"):
            vm.send(receiver, "echo:", [])

    def test_missing_selector_raises_message_not_understood(self) -> None:
        image, vm = bootstrap_image()
        receiver = vm.send(image.class_named("Object"), "new", [])
        with self.assertRaisesRegex(MessageNotUnderstood, "does not understand #frob"):
            vm.send(receiver, "frob", [])

    def test_dead_home_context_raises_invalid_non_local_return(self) -> None:
        image, vm = bootstrap_image()
        image.install_method("Object", compile_method("returningBlock\n    ^[ ^1 ]", "Object", []))

        with self.assertRaisesRegex(InvalidNonLocalReturn, "dead home context"):
            vm.evaluate("| block | block := self returningBlock. block value")

    def test_image_manifest_round_trip(self) -> None:
        image, _vm = bootstrap_image()
        with tempfile.NamedTemporaryFile(suffix=".json") as handle:
            save_image_manifest(image, handle.name)
            loaded_image, loaded_vm = load_image_manifest(handle.name)
            self.assertEqual(loaded_vm.evaluate("4 factorial"), 24)
            self.assertIsNotNone(loaded_image.class_named("Object").lookup("yourself"))


if __name__ == "__main__":
    unittest.main()
