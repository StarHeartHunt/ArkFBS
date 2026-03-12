# Ark FBS

This is a standalone PyPI package that provides a thin `pybind11` wrapper around
`flatbuffers::Parser` and `GenerateText`:

- load `.fbs` / `.bfbs`
- JSON -> FlatBuffer binary
- FlatBuffer binary -> JSON

## Installation

### Install from PyPI

```bash
python -m pip install -U ark-fbs
```

## Repo layout

- `cpp/`: C++ binding source (pybind11)
- `src/ark_fbs/`: Python package wrapper + typing stubs
- `third_party/flatbuffers/`: **your fork** of FlatBuffers as a git submodule (recommended)
- `third_party/nlohmann_json/`: single-header `nlohmann/json.hpp` (required by your FlatBuffers fork)

## Local build (editable)

```bash
uv sync --group dev
uv pip install -e .
```

## Usage

```python
import ark_fbs

s = ark_fbs.Schema.from_fbs_text(
    "namespace t; table A { x:int; } root_type A;"
)
b = s.json_to_binary('{"x":1}')
print(s.binary_to_json(b))
```

### Load from a `.fbs` file (with include paths)

```python
import ark_fbs

schema = ark_fbs.Schema.from_fbs_file(
    "schema.fbs",
    include_paths=["./includes", "./third_party/schemas"],
)
data = schema.json_to_binary('{"x": 123}')
print(schema.binary_to_json(data))
```

### Options and root type override

```python
import ark_fbs

opts = ark_fbs.Options()
opts.strict_json = True
opts.defaults_json = True
opts.size_prefixed = False

schema = ark_fbs.Schema.from_fbs_text(
    "namespace t; table A { x:int; } root_type A;",
    options=opts,
    root_type_override="A",
)
```

### Load from `.bfbs` (serialized schema)

```python
import ark_fbs

schema = ark_fbs.Schema.from_fbs_text("namespace t; table A { x:int; } root_type A;")
bfbs = schema.serialize_schema_bfbs()

schema2 = ark_fbs.Schema.from_bfbs(bfbs)
print(schema2.binary_to_json(schema2.json_to_binary('{"x": 1}')))
```

## API Reference

### `ark_fbs.Options`

`Options()` controls how schemas are parsed and how JSON/text is emitted.

Fields (all are `bool`):

- `strict_json` (default `True`): Require strict JSON input when parsing JSON.
- `natural_utf8` (default `True`): Emit/interpret UTF-8 in a “natural” way (FlatBuffers option).
- `defaults_json` (default `True`): Include default scalar values in emitted JSON.
- `size_prefixed` (default `False`): Treat binary buffers as size-prefixed.
- `output_enum_identifiers` (default `True`): Output enum identifiers rather than numeric values.

### `ark_fbs.Schema`

`Schema` wraps a `flatbuffers::Parser` instance.

#### Constructors

- `Schema.from_fbs_file(schema_path: str, include_paths: list[str] = [], options: Options = Options(), root_type_override: str = "") -> Schema`
  - **schema_path**: Path to the `.fbs` file.
  - **include_paths**: Extra include directories for `include "foo.fbs"` resolution.
  - **options**: Parsing/JSON options.
  - **root_type_override**: If non-empty, forces the root type (overrides `root_type` in schema).
  - **Raises**: `ValueError` if the schema file cannot be loaded, parsing fails, or `root_type_override` is unknown.

- `Schema.from_fbs_text(schema_text: str, include_paths: list[str] = [], options: Options = Options(), source_filename: str = "", root_type_override: str = "") -> Schema`
  - **schema_text**: The schema content (as text).
  - **source_filename**: Optional filename used in error messages and for include resolution context.
  - **Raises**: `ValueError` on parse errors or unknown root type override.

- `Schema.from_bfbs(bfbs: bytes, options: Options = Options(), root_type_override: str = "") -> Schema`
  - **bfbs**: Serialized schema bytes (`.bfbs`).
  - **Raises**: `ValueError` if deserialization fails or `root_type_override` is unknown.

#### Methods

- `Schema.json_to_binary(json: str) -> bytes`
  - Parses JSON using the loaded schema and returns the FlatBuffer binary.
  - **Raises**: `ValueError` if JSON parsing fails, or if no root type is set.

- `Schema.binary_to_json(data: bytes) -> str`
  - Converts a FlatBuffer binary (for the schema's root type) to JSON text.
  - **Raises**: `ValueError` if conversion fails, or if no root type is set.

- `Schema.serialize_schema_bfbs() -> bytes`
  - Serializes the currently loaded schema into `.bfbs` bytes.
  - **Raises**: `ValueError` if the schema is not initialized.

#### Root type behavior

`Schema.json_to_binary()` and `Schema.binary_to_json()` require a root type. You must either:

- Provide `root_type` in the schema (`root_type A;`), or
- Pass `root_type_override="A"` when constructing the `Schema`.

### Wheels / PyPI

CI uses `cibuildwheel` to produce wheels for multiple CPython versions and
architectures. See `.github/workflows/wheels.yml`.
