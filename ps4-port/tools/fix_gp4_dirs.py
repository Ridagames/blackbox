#!/usr/bin/env python3
"""Build a complete GP4 rootdir tree from the generated file entries."""
import sys
import xml.etree.ElementTree as ET

def main(path):
    tree = ET.parse(path)
    project = tree.getroot()
    files_node = project.find("files")
    if files_node is None:
        raise RuntimeError("GP4 project has no files node")
    rootdir = project.find("rootdir")
    if rootdir is None:
        rootdir = ET.SubElement(project, "rootdir")

    # Rebuild from scratch to avoid duplicate or cyclic directory nodes.
    for child in list(rootdir):
        rootdir.remove(child)

    directories = {}
    for file_node in list(files_node.findall("file")):
        target = file_node.get("targ_path", "").replace("\\", "/").strip("/")
        if not target:
            continue
        parts = [part for part in target.split("/")[:-1] if part and part != "."]
        if len(parts) > 3:
            print("Skipping malformed GP4 target path:", target[:240])
            files_node.remove(file_node)
            continue
        parent = rootdir
        prefix = []
        for part in parts:
            if part == "..":
                raise RuntimeError("Unsafe parent directory in GP4 path")
            prefix.append(part)
            key = tuple(prefix)
            if key not in directories:
                directories[key] = ET.SubElement(parent, "dir", {"targ_name": part})
            parent = directories[key]

    # Keep the generated XML compact. The directory tree is deliberately shallow
    # (sce_sys/about and data/covers), so no recursive pretty-printer is needed.
    tree.write(path, encoding="utf-8", xml_declaration=True)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: fix_gp4_dirs.py <project.gp4>")
    main(sys.argv[1])
