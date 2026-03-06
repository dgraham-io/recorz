"""Recorz Phase 1 seed runtime."""

from .bootstrap import bootstrap_image
from .vm import VirtualMachine

__all__ = ["VirtualMachine", "bootstrap_image"]
