#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace py = pybind11;

namespace {

struct Options {
  bool strict_json = true;
  bool natural_utf8 = true;
  bool defaults_json = true;
  bool size_prefixed = false;
  bool output_enum_identifiers = false;
};

flatbuffers::IDLOptions ToIdlOptions(const Options &o) {
  flatbuffers::IDLOptions opts;
  opts.strict_json = o.strict_json;
  opts.natural_utf8 = o.natural_utf8;
  opts.output_default_scalars_in_json = o.defaults_json;
  opts.size_prefixed = o.size_prefixed;
  opts.output_enum_identifiers = o.output_enum_identifiers;
  opts.lang_to_generate = flatbuffers::IDLOptions::kJson;
  return opts;
}

class Schema {
 public:
  static Schema FromFbsFile(const std::string &schema_path,
                            const std::vector<std::string> &include_paths,
                            const Options &options,
                            const std::string &root_type_override) {
    std::string schema_text;
    if (!flatbuffers::LoadFile(schema_path.c_str(), /*binary=*/false,
                               &schema_text)) {
      throw py::value_error("could not load schema file: " + schema_path);
    }
    return FromFbsText(schema_text, include_paths, options, schema_path,
                       root_type_override);
  }

  static Schema FromFbsText(const std::string &schema_text,
                            const std::vector<std::string> &include_paths,
                            const Options &options,
                            const std::string &source_filename,
                            const std::string &root_type_override) {
    Schema s;
    s.parser_ = std::make_unique<flatbuffers::Parser>();
    s.parser_->opts = ToIdlOptions(options);

    std::vector<const char *> c_includes;
    c_includes.reserve(include_paths.size() + 1);
    for (const auto &p : include_paths) c_includes.push_back(p.c_str());
    c_includes.push_back(nullptr);

    const char *filename =
        source_filename.empty() ? nullptr : source_filename.c_str();
    if (!s.parser_->Parse(schema_text.c_str(),
                          c_includes.empty() ? nullptr : c_includes.data(),
                          filename)) {
      throw py::value_error(s.parser_->error_);
    }

    if (!root_type_override.empty()) {
      if (!s.parser_->SetRootType(root_type_override.c_str())) {
        throw py::value_error("unknown root type: " + root_type_override);
      }
    }
    return s;
  }

  static Schema FromBfbs(py::bytes bfbs, const Options &options,
                         const std::string &root_type_override) {
    Schema s;
    s.parser_ = std::make_unique<flatbuffers::Parser>();
    s.parser_->opts = ToIdlOptions(options);

    const std::string buf = bfbs;
    if (!s.parser_->Deserialize(
            reinterpret_cast<const uint8_t *>(buf.data()), buf.size())) {
      throw py::value_error("failed to deserialize bfbs schema");
    }

    if (!root_type_override.empty()) {
      if (!s.parser_->SetRootType(root_type_override.c_str())) {
        throw py::value_error("unknown root type: " + root_type_override);
      }
    }
    return s;
  }

  std::string binary_to_json(py::bytes data) const {
    EnsureReady();
    const std::string buf = data;

    std::string out;
    const char *err = flatbuffers::GenerateText(
        *parser_, reinterpret_cast<const void *>(buf.data()), &out);
    if (err) throw py::value_error(std::string("GenerateText failed: ") + err);
    return out;
  }

  py::bytes json_to_binary(const std::string &json) const {
    EnsureReady();
    parser_->builder_.Clear();
    if (!parser_->ParseJson(json.c_str())) {
      throw py::value_error(parser_->error_);
    }
    return py::bytes(
        reinterpret_cast<const char *>(parser_->builder_.GetBufferPointer()),
        static_cast<py::ssize_t>(parser_->builder_.GetSize()));
  }

  py::bytes serialize_schema_bfbs() const {
    if (!parser_) throw py::value_error("schema is not initialized");
    parser_->builder_.Clear();
    parser_->Serialize();
    return py::bytes(
        reinterpret_cast<const char *>(parser_->builder_.GetBufferPointer()),
        static_cast<py::ssize_t>(parser_->builder_.GetSize()));
  }

 private:
  void EnsureReady() const {
    if (!parser_) throw py::value_error("schema is not initialized");
    if (!parser_->root_struct_def_) {
      throw py::value_error(
          "no root type set (schema missing 'root_type' and no override "
          "provided)");
    }
  }

  std::unique_ptr<flatbuffers::Parser> parser_;
};

}  // namespace

PYBIND11_MODULE(_flatbuffers_idl, m) {
  m.doc() = "FlatBuffers schema parsing + JSON<->binary helpers (C++ Parser)";

  py::class_<Options>(m, "Options")
      .def(py::init<>())
      .def_readwrite("strict_json", &Options::strict_json)
      .def_readwrite("natural_utf8", &Options::natural_utf8)
      .def_readwrite("defaults_json", &Options::defaults_json)
      .def_readwrite("size_prefixed", &Options::size_prefixed)
      .def_readwrite("output_enum_identifiers",
                     &Options::output_enum_identifiers);

  py::class_<Schema>(m, "Schema")
      .def_static(
          "from_fbs_file",
          &Schema::FromFbsFile,
          py::arg("schema_path"),
          py::arg("include_paths") = std::vector<std::string>{},
          py::arg("options") = Options{},
          py::arg("root_type_override") = std::string{})
      .def_static(
          "from_fbs_text",
          &Schema::FromFbsText,
          py::arg("schema_text"),
          py::arg("include_paths") = std::vector<std::string>{},
          py::arg("options") = Options{},
          py::arg("source_filename") = std::string{},
          py::arg("root_type_override") = std::string{})
      .def_static(
          "from_bfbs",
          &Schema::FromBfbs,
          py::arg("bfbs"),
          py::arg("options") = Options{},
          py::arg("root_type_override") = std::string{})
      .def("binary_to_json", &Schema::binary_to_json, py::arg("data"))
      .def("json_to_binary", &Schema::json_to_binary, py::arg("json"))
      .def("serialize_schema_bfbs", &Schema::serialize_schema_bfbs);
}

