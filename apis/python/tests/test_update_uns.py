from copy import deepcopy
from dataclasses import dataclass
from typing import Optional

import pandas as pd

import tiledbsoma
from tiledbsoma import Experiment
from tiledbsoma.io._common import UnsMapping
from tiledbsoma.io.ingest import update_uns

from tests._util import assert_uns_equal, make_df
from tests.parametrize_cases import Err, err_ctx, parametrize_cases
from tests.test_basic_anndata_io import TEST_UNS, make_uns_adata


@dataclass
class Case:
    id: str
    uns_updates: UnsMapping
    update_err: Optional[Err] = None
    outgest_err: Optional[Err] = None


def case(
    id: str,
    update_err: Optional[Err] = None,
    outgest_err: Optional[Err] = None,
    **uns_updates,
) -> Case:
    return Case(
        id=id,
        uns_updates=uns_updates,
        update_err=update_err,
        outgest_err=outgest_err,
    )


# fmt: off
@parametrize_cases(
    [
        case(
            "Update scalar values, add a new DataFrame",
            int_scalar=11,
            float_scalar=2.2,
            string_scalar="HELLO 2",
            new_df=pd.DataFrame({"a": [1, 2, 3]}),
        ),
        case(
            "Update one scalar, overwrite a DataFrame with an identical DataFrame",
            int_scalar=11,
            pd_df_indexed=TEST_UNS["pd_df_indexed"].copy(),
        ),
        case(
            "Update DataFrame values",
            pd_df_indexed=TEST_UNS["pd_df_indexed"].assign(column_1=list("ghi")),
        ),
        case(
            "Append rows to a DataFrame",
            pd_df_indexed=make_df("1,2,3,4,5", column_1="d,e,f,g,h"),
            update_err=(ValueError, "update_uns_dict pd_df_indexed: old and new data must have the same row count; got 3 != 5"),
        ),
        case(
            "Add an index.name to DataFrame that previously had an unnamed index",
            pd_df_indexed=TEST_UNS["pd_df_indexed"].reset_index(names="idx").set_index("idx"),
            update_err=(ValueError, r"update_uns_dict pd_df_indexed: columns don't match, missing \[index\], adding \[idx\]"),
        ),
        case(
            "Update a Collection to a DF",
            strings=make_df("a,b,c", column_1="d,e,f"),
            update_err=(ValueError, r"expected DataFrame at file:///[^\s]+, found Collection"),
        ),
        case(
            "Update a scalar Collection to a DF",
            int_scalar=make_df("a,b,c", column_1="d,e,f"),
            update_err=(ValueError, r"expected DataFrame at file:///[^\s]+, found int"),
        ),
    ],
)
# fmt: on
def test_update_uns(tmp_path, uns_updates, update_err, outgest_err):
    soma_uri, adata = make_uns_adata(tmp_path)

    if isinstance(uns_updates, tuple):
        uns_updates, patches = uns_updates
    else:
        patches = {}

    with Experiment.open(soma_uri, "w") as exp:
        with err_ctx(update_err):
            update_uns(exp, uns_updates, measurement_name="RNA")

    with Experiment.open(soma_uri) as exp:
        with err_ctx(outgest_err):
            adata2 = tiledbsoma.io.to_anndata(exp, measurement_name="RNA")

    expected = deepcopy(TEST_UNS)
    for k, v in uns_updates.items():
        expected[k] = patches[k] if k in patches else v
    assert_uns_equal(adata2.uns, expected)
