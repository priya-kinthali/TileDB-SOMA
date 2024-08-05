import json
from typing import Any, Dict, Sequence, Tuple

from somacore import coordinates
from typing_extensions import Self


class Axis(coordinates.Axis):
    """A description of an axis of a coordinate system

    TODO: Note if this class remains more or less as is the base class in somacore
    can be implemented as a ``dataclasses.dataclass``.

    Lifecycle: experimental
    """

    @classmethod
    def from_json(cls, data: str) -> Self:
        """Create from a json blob.

        Args:
           data: json blob to deserialize.

        Lifecycle: experimental
        """
        kwargs = json.loads(data)
        if not isinstance(kwargs, dict):
            raise ValueError()
        return cls(**kwargs)

    def to_dict(self) -> Dict[str, Any]:
        """TODO: Add docstring"""
        kwargs: Dict[str, Any] = {"name": self.name}
        if self.units is not None:
            kwargs["units"] = self.units
        if self.scale is not None:
            kwargs["scale"] = self.scale
        return kwargs

    def to_json(self) -> str:
        """TODO: Add docstring"""
        return json.dumps(self.to_dict())


class CoordinateSpace(coordinates.CoordinateSpace):
    """A coordinate system for spatial data."""

    @classmethod
    def from_json(cls, data: str) -> Self:
        """TODO: Add docstring"""
        # TODO: Needs good, comprehensive error handling.
        raw = json.loads(data)
        return cls(tuple(Axis(**axis) for axis in raw))

    def __init__(self, axes: Sequence[Axis]):
        """TODO: Add docstring"""
        # TODO: Needs good, comprehensive error handling.
        self._axes = tuple(axes)

    def __len__(self) -> int:
        return len(self._axes)

    def __getitem__(self, index: int) -> Axis:
        return self._axes[index]

    @property
    def axes(self) -> Tuple[Axis, ...]:
        """TODO: Add docstring"""
        return self._axes

    def to_json(self) -> str:
        """TODO: Add docstring"""
        return json.dumps(tuple(axis.to_dict() for axis in self._axes))
