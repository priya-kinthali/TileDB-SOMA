import pyarrow as pa

import tiledbsoma as t


def test_soma_indexed_dataframe(tmp_path):
    sdf = t.SOMAIndexedDataFrame(uri=tmp_path.as_posix())

    asch = pa.schema(
        [
            ("foo", pa.int32()),
            ("bar", pa.float64()),
            ("baz", pa.string()),
        ]
    )

    # Create
    sdf.create(schema=asch, index_column_names=["foo"])

    # Write
    for _ in range(3):
        pydict = {}
        pydict["foo"] = [10, 20, 30, 40, 50]
        pydict["bar"] = [4.1, 5.2, 6.3, 7.4, 8.5]
        pydict["baz"] = ["apple", "ball", "cat", "dog", "egg"]
        rb = pa.RecordBatch.from_pydict(pydict)
        sdf.write(rb)

    # Read all
    batch = sdf.read_all()
    # Weird thing about pyarrow RecordBatch:
    # * We should have 5 "rows" with 3 "columns"
    # * Indeed batch.num_rows is 5 and batch.num_columns is 3
    # * But len(batch) is 3
    # * If you thought `for record in record_batch` would print records ... you would be wrong -- it
    #   loops over columns
    assert batch.num_rows == 5
    # We should be getting back the soma_rowid column as well
    assert batch.num_columns == 4
    assert [e.as_py() for e in list(batch["soma_rowid"])] == [0, 1, 2, 3, 4]
    assert [e.as_py() for e in list(batch["foo"])] == pydict["foo"]
    assert [e.as_py() for e in list(batch["bar"])] == pydict["bar"]
    assert [e.as_py() for e in list(batch["baz"])] == pydict["baz"]

    # Read ids
    batch = sdf.read_all(ids=[30, 10])
    # Weird thing about pyarrow RecordBatch:
    # * We should have 5 "rows" with 3 "columns"
    # * Indeed batch.num_rows is 5 and batch.num_columns is 3
    # * But len(batch) is 3
    # * If you thought `for record in record_batch` would print records ... you would be wrong -- it
    #   loops over columns
    assert batch.num_rows == 2
    # We should be getting back the soma_rowid column as well
    assert batch.num_columns == 4
    assert sorted([e.as_py() for e in list(batch["soma_rowid"])]) == [0, 2]
    assert sorted([e.as_py() for e in list(batch["foo"])]) == [10, 30]
    assert sorted([e.as_py() for e in list(batch["bar"])]) == [4.1, 6.3]
    assert sorted([e.as_py() for e in list(batch["baz"])]) == ["apple", "cat"]
