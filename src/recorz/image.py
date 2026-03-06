"""JSON manifest persistence for the Phase 1 seed image.

This is intentionally a structural image manifest rather than a raw heap dump.
It preserves the class universe and method set, then rebuilds a fresh image from
that manifest on load. That keeps the bootstrap explicit while leaving room for
fuller snapshots later.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Tuple, Union

from .bootstrap import _compile_and_install
from .model import CompiledMethod, RuntimeImage, RecorzClass
from .vm import VirtualMachine


def save_image_manifest(image: RuntimeImage, path: Union[str, Path]) -> None:
    payload = {
        "classes": [
            {
                "name": recorz_class.name,
                "superclass": recorz_class.superclass.name if recorz_class.superclass is not None else None,
                "instance_variables": recorz_class.instance_variables,
                "is_metaclass": recorz_class.is_metaclass,
            }
            for recorz_class in image.classes.values()
        ],
        "methods": [
            {
                "class": class_name,
                "selector": method.selector,
                "primitive": method.primitive,
                "arg_names": method.arg_names,
                "source": method.source,
            }
            for class_name, recorz_class in sorted(image.classes.items())
            for method in recorz_class.methods.values()
        ],
    }
    Path(path).write_text(json.dumps(payload, indent=2, sort_keys=True))


def load_image_manifest(path: Union[str, Path]) -> Tuple[RuntimeImage, VirtualMachine]:
    from .bootstrap import bootstrap_image

    image, vm = bootstrap_image()
    payload = json.loads(Path(path).read_text())

    image.classes.clear()
    image.globals.clear()
    for class_spec in payload["classes"]:
        image.classes[class_spec["name"]] = RecorzClass(
            name=class_spec["name"],
            instance_variables=list(class_spec["instance_variables"]),
            is_metaclass=class_spec["is_metaclass"],
        )
    for class_spec in payload["classes"]:
        superclass_name = class_spec["superclass"]
        if superclass_name is not None:
            image.classes[class_spec["name"]].superclass = image.classes[superclass_name]
    for recorz_class in image.classes.values():
        image.globals[recorz_class.name] = recorz_class
    for recorz_class in image.classes.values():
        if recorz_class.is_metaclass:
            continue
        recorz_class.metaclass = image.classes[f"{recorz_class.name} class"]

    for method_spec in payload["methods"]:
        if method_spec["primitive"] is not None:
            image.install_method(
                method_spec["class"],
                CompiledMethod(
                    selector=method_spec["selector"],
                    defining_class=method_spec["class"],
                    arg_names=list(method_spec["arg_names"]),
                    temp_names=[],
                    instructions=[],
                    literals=[],
                    source=method_spec["source"],
                    primitive=method_spec["primitive"],
                ),
            )
            continue
        _compile_and_install(image, method_spec["class"], method_spec["source"])
    return image, vm
