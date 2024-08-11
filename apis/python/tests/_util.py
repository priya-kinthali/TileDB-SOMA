from contextlib import contextmanager, nullcontext
from pathlib import Path
from typing import Any, List, Optional, Tuple, Type, Union

import pandas as pd
import pytest
from _pytest._code import ExceptionInfo
from _pytest.python_api import E, RaisesContext
from typeguard import suppress_type_checks


def parse_col(col_str: str) -> Tuple[Optional[str], List[str]]:
    """Parse a "column string" of the form `val1,val2,...` or `name=val1,val2,...`."""
    pcs = col_str.split("=")
    if len(pcs) == 1:
        return None, col_str.split(",")
    elif len(pcs) == 2:
        name, vals_str = pcs
        vals = vals_str.split(",")
        return name, vals
    else:
        raise ValueError(f"Invalid column string: {col_str}")


def make_df(index_str: Optional[str] = None, **cols) -> pd.DataFrame:
    """DataFrame construction helper, for tests.

    - index and columns are provided as strings of the form `name=val1,val2,...`.
    - `name=` is optional for the initial (`index_str`) arg.
    """
    cols = dict([(col, parse_col(col_str)[1]) for col, col_str in cols.items()])
    index = None
    index_name = None
    if index_str:
        index_name, index = parse_col(index_str)
    df = pd.DataFrame(cols, index=index)
    df.index.name = index_name
    return df


HERE = Path(__file__).parent
PY_ROOT = HERE.parent
TESTDATA = PY_ROOT / "testdata"


@contextmanager
def raises_no_typeguard(exc: Type[Exception], *args: Any, **kwargs: Any):
    """
    Temporarily suppress typeguard checks in order to verify a runtime exception is raised.

    Otherwise, most errors end up manifesting as ``TypeCheckError``s, during tests (thanks to
    ``typeguard``'s import hook).
    """
    with suppress_type_checks():
        with pytest.raises(exc, *args, **kwargs):
            yield


def maybe_raises(
    expected_exception: Union[None, Type[E], Tuple[Type[E], ...]],
    *args: Any,
    **kwargs: Any,
) -> Union[RaisesContext[E], ExceptionInfo[E]]:
    """
    Wrapper around ``pytest.raises`` that accepts None (signifying no exception should be raised).
    This is only necessary since ``pytest.raises`` does not itself accept None, so we are
    decorating.

    Useful in test cases that are parameterized to test both valid and invalid inputs.
    """
    return (
        nullcontext()
        if expected_exception is None
        else pytest.raises(expected_exception, *args, **kwargs)
    )
