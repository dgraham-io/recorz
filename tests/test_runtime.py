import tempfile
import unittest

from recorz.bootstrap import bootstrap_image
from recorz.image import load_image_manifest, save_image_manifest


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

    def test_image_manifest_round_trip(self) -> None:
        image, _vm = bootstrap_image()
        with tempfile.NamedTemporaryFile(suffix=".json") as handle:
            save_image_manifest(image, handle.name)
            loaded_image, loaded_vm = load_image_manifest(handle.name)
            self.assertEqual(loaded_vm.evaluate("4 factorial"), 24)
            self.assertIsNotNone(loaded_image.class_named("Object").lookup("yourself"))


if __name__ == "__main__":
    unittest.main()
