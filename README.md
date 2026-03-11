# Ark FBS

This is a standalone PyPI package that provides a thin `pybind11` wrapper around
`flatbuffers::Parser` and `GenerateText`:

- load `.fbs` / `.bfbs`
- JSON -> FlatBuffer binary
- FlatBuffer binary -> JSON

## Repo layout

- `cpp/`: C++ binding source (pybind11)
- `src/flatbuffers_idl/`: Python package wrapper + typing stubs
- `third_party/flatbuffers/`: **your fork** of FlatBuffers as a git submodule (recommended)

## Local build (editable)

```bash
uv sync --group dev
uv pip install -e .
```

## Usage

```python
import flatbuffers_idl

s = flatbuffers_idl.Schema.from_fbs_text(
    "namespace t; table A { x:int; } root_type A;"
)
b = s.json_to_binary('{"x":1}')
print(s.binary_to_json(b))
```

### Wheels / PyPI

CI uses `cibuildwheel` to produce wheels for multiple CPython versions and
architectures. See `.github/workflows/wheels.yml`.
