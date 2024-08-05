import numpy as np
import pytest

import tiledbsoma as soma


@pytest.mark.parametrize(
    "original",
    [
        soma.Axis(name="dim0"),
        soma.Axis(name="dim0", units="micrometer"),
        soma.Axis(name="dim0", units="nanometer", scale=np.float64(65.0)),
    ],
)
def test_axis_json_roundtrip(original: soma.Axis):
    json_blob = original.to_json()
    result = soma.Axis.from_json(json_blob)
    assert result.name == original.name
    assert result.units == original.units
    assert result.scale == original.scale


@pytest.mark.parametrize(
    "original",
    [
        soma.CoordinateSpace((soma.Axis(name="dim0", units="meter"),)),
        soma.CoordinateSpace(
            (
                soma.Axis(name="dim0", units="micrometer"),
                soma.Axis(name="dim1"),
                soma.Axis(name="dim2", units="micrometer", scale=np.float64(65.0)),
            ),
        ),
        soma.CoordinateSpace([]),
    ],
)
def test_coordinate_system_json_roundtrip(original: soma.CoordinateSpace):
    json_blob = original.to_json()
    result = soma.CoordinateSpace.from_json(json_blob)
    assert len(result) == len(original)
    for index in range(len(result)):
        assert result[index] == original[index]
