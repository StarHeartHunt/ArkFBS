from __future__ import annotations

from typing import List

class Options:
    strict_json: bool
    natural_utf8: bool
    defaults_json: bool
    size_prefixed: bool
    output_enum_identifiers: bool

    def __init__(
        self,
        *,
        strict_json: bool = True,
        natural_utf8: bool = True,
        defaults_json: bool = True,
        size_prefixed: bool = False,
        output_enum_identifiers: bool = True,
    ) -> None: ...

class Schema:
    @staticmethod
    def from_fbs_file(
        schema_path: str,
        include_paths: List[str] = ...,
        options: Options = ...,
        root_type_override: str = ...,
    ) -> Schema: ...

    @staticmethod
    def from_fbs_text(
        schema_text: str,
        include_paths: List[str] = ...,
        options: Options = ...,
        source_filename: str = ...,
        root_type_override: str = ...,
    ) -> Schema: ...

    @staticmethod
    def from_bfbs(
        bfbs: bytes,
        options: Options = ...,
        root_type_override: str = ...,
    ) -> Schema: ...

    def binary_to_json(self, data: bytes) -> str: ...
    def json_to_binary(self, json: str) -> bytes: ...
    def serialize_schema_bfbs(self) -> bytes: ...

