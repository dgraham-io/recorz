"""Bootstrap the smallest coherent Recorz image."""

from __future__ import annotations

from .compiler import compile_method
from .model import CompiledMethod, RuntimeImage, RecorzClass
from .vm import (
    VirtualMachine,
    primitive_block_value,
    primitive_class,
    primitive_class_name,
    primitive_class_superclass,
    primitive_dnu,
    primitive_equals,
    primitive_false_if_false,
    primitive_false_if_true,
    primitive_false_if_true_if_false,
    primitive_false_not,
    primitive_identity,
    primitive_new,
    primitive_print_string,
    primitive_small_integer_add,
    primitive_small_integer_multiply,
    primitive_small_integer_subtract,
    primitive_true_if_false,
    primitive_true_if_true,
    primitive_true_if_true_if_false,
    primitive_true_not,
)


def _primitive_method(selector: str, primitive: str, defining_class: str, arg_names: list[str]) -> CompiledMethod:
    return CompiledMethod(
        selector=selector,
        defining_class=defining_class,
        arg_names=arg_names,
        temp_names=[],
        instructions=[],
        literals=[],
        source="",
        primitive=primitive,
    )


def bootstrap_image() -> tuple[RuntimeImage, VirtualMachine]:
    image = RuntimeImage()
    ordinary_specs = [
        ("Object", None, []),
        ("Behavior", "Object", []),
        ("ClassDescription", "Behavior", []),
        ("Class", "ClassDescription", []),
        ("Metaclass", "ClassDescription", []),
        ("UndefinedObject", "Object", []),
        ("Boolean", "Object", []),
        ("True", "Boolean", []),
        ("False", "Boolean", []),
        ("Magnitude", "Object", []),
        ("Number", "Magnitude", []),
        ("Integer", "Number", []),
        ("SmallInteger", "Integer", []),
        ("String", "Object", []),
        ("Symbol", "Object", []),
        ("BlockClosure", "Object", []),
        ("CompiledMethod", "Object", []),
        ("Context", "Object", []),
        ("Process", "Object", []),
    ]

    for name, _superclass_name, ivars in ordinary_specs:
        image.classes[name] = RecorzClass(name=name, instance_variables=list(ivars))
    for name, superclass_name, _ivars in ordinary_specs:
        if superclass_name is not None:
            image.classes[name].superclass = image.class_named(superclass_name)

    for name, superclass_name, _ivars in ordinary_specs:
        metaclass_name = f"{name} class"
        image.classes[metaclass_name] = RecorzClass(name=metaclass_name, is_metaclass=True)
        image.classes[name].metaclass = image.class_named(metaclass_name)
        if superclass_name is not None:
            image.classes[metaclass_name].superclass = image.class_named(f"{superclass_name} class")

    for name, recorz_class in image.classes.items():
        image.globals[name] = recorz_class

    vm = VirtualMachine(image)
    vm.register_primitive("class", primitive_class)
    vm.register_primitive("new", primitive_new)
    vm.register_primitive("className", primitive_class_name)
    vm.register_primitive("classSuperclass", primitive_class_superclass)
    vm.register_primitive("identity", primitive_identity)
    vm.register_primitive("equals", primitive_equals)
    vm.register_primitive("printString", primitive_print_string)
    vm.register_primitive("doesNotUnderstand", primitive_dnu)
    vm.register_primitive("smallIntegerAdd", primitive_small_integer_add)
    vm.register_primitive("smallIntegerSubtract", primitive_small_integer_subtract)
    vm.register_primitive("smallIntegerMultiply", primitive_small_integer_multiply)
    vm.register_primitive("trueIfTrue", primitive_true_if_true)
    vm.register_primitive("trueIfFalse", primitive_true_if_false)
    vm.register_primitive("falseIfTrue", primitive_false_if_true)
    vm.register_primitive("falseIfFalse", primitive_false_if_false)
    vm.register_primitive("trueIfTrueIfFalse", primitive_true_if_true_if_false)
    vm.register_primitive("falseIfTrueIfFalse", primitive_false_if_true_if_false)
    vm.register_primitive("trueNot", primitive_true_not)
    vm.register_primitive("falseNot", primitive_false_not)
    vm.register_primitive("blockValue", primitive_block_value)

    _install_primitives(image)
    _install_source_methods(image)
    return image, vm


def _install_primitives(image: RuntimeImage) -> None:
    image.install_method("Object", _primitive_method("class", "class", "Object", []))
    image.install_method("Object", _primitive_method("==", "identity", "Object", ["other"]))
    image.install_method("Object", _primitive_method("=", "equals", "Object", ["other"]))
    image.install_method("Object", _primitive_method("printString", "printString", "Object", []))
    image.install_method("Object", _primitive_method("doesNotUnderstand:", "doesNotUnderstand", "Object", ["selector"]))

    image.install_method("Object class", _primitive_method("new", "new", "Object class", []))
    image.install_method("Object class", _primitive_method("name", "className", "Object class", []))
    image.install_method("Object class", _primitive_method("superclass", "classSuperclass", "Object class", []))

    image.install_method("SmallInteger", _primitive_method("+", "smallIntegerAdd", "SmallInteger", ["other"]))
    image.install_method("SmallInteger", _primitive_method("-", "smallIntegerSubtract", "SmallInteger", ["other"]))
    image.install_method("SmallInteger", _primitive_method("*", "smallIntegerMultiply", "SmallInteger", ["other"]))

    image.install_method("True", _primitive_method("ifTrue:", "trueIfTrue", "True", ["trueBlock"]))
    image.install_method("True", _primitive_method("ifFalse:", "trueIfFalse", "True", ["falseBlock"]))
    image.install_method("True", _primitive_method("ifTrue:ifFalse:", "trueIfTrueIfFalse", "True", ["trueBlock", "falseBlock"]))
    image.install_method("True", _primitive_method("not", "trueNot", "True", []))

    image.install_method("False", _primitive_method("ifTrue:", "falseIfTrue", "False", ["trueBlock"]))
    image.install_method("False", _primitive_method("ifFalse:", "falseIfFalse", "False", ["falseBlock"]))
    image.install_method("False", _primitive_method("ifTrue:ifFalse:", "falseIfTrueIfFalse", "False", ["trueBlock", "falseBlock"]))
    image.install_method("False", _primitive_method("not", "falseNot", "False", []))

    image.install_method("BlockClosure", _primitive_method("value", "blockValue", "BlockClosure", []))
    image.install_method("BlockClosure", _primitive_method("value:", "blockValue", "BlockClosure", ["arg1"]))


def _install_source_methods(image: RuntimeImage) -> None:
    _compile_and_install(
        image,
        "Object",
        """yourself
    ^self
""",
    )
    _compile_and_install(
        image,
        "Object",
        """isNil
    ^false
""",
    )
    _compile_and_install(
        image,
        "Object",
        """notNil
    ^true
""",
    )
    _compile_and_install(
        image,
        "UndefinedObject",
        """isNil
    ^true
""",
    )
    _compile_and_install(
        image,
        "UndefinedObject",
        """notNil
    ^false
""",
    )
    _compile_and_install(
        image,
        "SmallInteger",
        """factorial
    self = 0 ifTrue: [ ^1 ].
    ^self * (self - 1) factorial
""",
    )
    _compile_and_install(
        image,
        "Object",
        """captureExample
    | x |
    x := 1.
    [ x := x + 2. x ] value.
    ^x
""",
    )


def _compile_and_install(image: RuntimeImage, class_name: str, source: str) -> None:
    recorz_class = image.class_named(class_name)
    compiled = compile_method(source, class_name, recorz_class.all_instance_variables())
    image.install_method(class_name, compiled)
