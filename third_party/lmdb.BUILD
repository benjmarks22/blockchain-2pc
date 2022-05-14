# Description:
#   LMDB is the Lightning Memory-mapped Database.

licenses(["notice"])  # OpenLDAP Public License

exports_files(["LICENSE"])

cc_library(
    name = "lmdb",
    srcs = [
        "mdb.c",
        "midl.c",
    ],
    hdrs = [
        "lmdb.h",
        "midl.h",
    ],
    copts = [
        "-w",
    ],
    # lmdbxx includes lmdb headers as `#include <lmdb.h>` internally.
    # Adding `includes` could make <angled> include work.
    includes = ["."],
    visibility = ["//visibility:public"],
)