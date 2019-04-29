import os
import glob
import sys
import re


def edit_header_file(filename):
    with open(filename, 'r+') as f:
        content = f.read()
        f.seek(0)
        f.truncate()
        # Rename file
        content_new = re.sub('(?s)Header file `(\w+/[^`]*)`', r'\1', content, flags=re.M)
        # Remove redundant markdown separators
        content_new = re.sub('-{5}\n\n(-{5}\n+)+', r'\1', content_new, flags=re.M)
        # Fix headings hierarchy
        content_new = re.sub('\n#(#+)', r'\n\1', content_new, flags=re.M)
        f.write("---\n"
                "layout: default\n"
                "parent: API index\n"
                "---\n\n")
        f.write(content_new)


def edit_index_file(filename):
    with open(filename, 'r+') as f:
        content = f.read()
        f.seek(0)
        f.truncate()
        f.write("---\n"
                "title: API index\n"
                "layout: default\n"
                "has_children: true\n"
                "permalink: /api\n"
                "nav_order: 99\n"
                "---\n\n")
        f.write(content)


def hide_file(filename):
    with open(filename, 'r+') as f:
        content = f.read()
        f.seek(0)
        f.truncate()
        f.write("---\n"
                "layout: default\n"
                "nav_exclude: true\n"
                "---\n\n")
        f.write(content)


def process(filename):
    head, tail = os.path.split(filename)
    print("Editing file {}".format(tail))
    if tail.startswith("doc_ultramarine__"):
        edit_header_file(filename)
    elif tail.startswith("standardese_"):
        if tail.endswith("entities.md"):
            edit_index_file(filename)
        else:
            hide_file(filename)


for filename in glob.glob(os.path.join(os.getcwd(), '*.md')):
    process(filename)
