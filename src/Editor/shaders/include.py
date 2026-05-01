import re
import sys
from os import getcwd
from os.path import join, abspath, dirname, basename


def load_source(folder: str, filename: str, vendor: str, dependencies=[]):
    path = abspath(join(folder, filename))

    if path in dependencies:
        return ""
    else:
        dependencies.append(path)

    source = ""

    f = open(path)
    lines = f.readlines()
    f.close()
    for line in lines:
        if match := re.search(r'#include\s*["|<](.*.[glsl|hlsl|metal])["|>]',
                              line, re.IGNORECASE):
            try:
                new_folder = join(folder, dirname(match.group(1)))
                new_dep = basename(match.group(1))
                source += load_source(new_folder, new_dep, vendor, dependencies)
            except:
                print(new_folder)
                new_folder = join(vendor, dirname(match.group(1)))
                new_dep = basename(match.group(1))
                source += load_source(new_folder, new_dep, vendor, dependencies)
        else:
            source += line

    return source


if __name__ == "__main__":
    print("USAGE: python include.py [filename] [vendordir] [outfile]")
    folder = getcwd()
    filename = sys.argv[1]
    source = load_source(folder, filename, sys.argv[2], [])
    out = sys.argv[3]
    with open(out, 'w') as f:
        f.write(source)
