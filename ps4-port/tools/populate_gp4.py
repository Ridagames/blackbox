#!/usr/bin/env python3
"""Populate a create-gp4 project with every staged BLACKBOX PS4 payload file.

OpenOrbis create-gp4's --files parser in this toolchain build only emitted the
first path. Keep create-gp4 for the package metadata, then write the complete
GP4 <files> list and matching <rootdir> tree here.
"""
import glob
import os
import sys
import xml.etree.ElementTree as ET

REQUIRED = [
    "eboot.bin",
    "sce_sys/about/right.sprx",
    "sce_sys/param.sfo",
    "sce_sys/icon0.png",
    "sce_module/libc.prx",
    "sce_module/libSceFios2.prx",
    "data/catalog.json",
    "data/ui-background.bmp",
    "data/ui-logo.bmp",
]

def main(gp4_path):
    paths = list(REQUIRED)
    paths.extend(sorted(glob.glob("data/covers/*.bmp")))
    if not paths:
        raise RuntimeError("No payload files found")
    missing = [p for p in paths if not os.path.isfile(p) or os.path.getsize(p) == 0]
    if missing:
        raise RuntimeError("Missing or empty GP4 payload files: " + ", ".join(missing[:20]))

    tree = ET.parse(gp4_path)
    project = tree.getroot()
    files_node = project.find("files")
    if files_node is None:
        raise RuntimeError("Generated GP4 has no <files> element")

    for child in list(files_node):
        files_node.remove(child)
    for path in paths:
        normalized = path.replace(os.sep, "/")
        ET.SubElement(files_node, "file", {
            "targ_path": normalized,
            "orig_path": normalized,
        })

    rootdir = project.find("rootdir")
    if rootdir is None:
        rootdir = ET.SubElement(project, "rootdir")
    for child in list(rootdir):
        rootdir.remove(child)

    directories = {}
    for path in paths:
        parts = path.replace(os.sep, "/").split("/")[:-1]
        parent = rootdir
        prefix = []
        for part in parts:
            prefix.append(part)
            key = tuple(prefix)
            if key not in directories:
                directories[key] = ET.SubElement(parent, "dir", {"targ_name": part})
            parent = directories[key]

    tree.write(gp4_path, encoding="utf-8", xml_declaration=True)
    print("BLACKBOX GP4 payload: %d files (%d covers)" %
          (len(paths), len(paths) - len(REQUIRED)))
    print("BLACKBOX GP4 manifest verified: all source files exist and are non-empty")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: populate_gp4.py <project.gp4>")
    main(sys.argv[1])
