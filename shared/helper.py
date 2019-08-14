# Helper script to create C++ code for grabbing desired MethodInfo/classes
import sys
import argparse
import os
import json

modes = ["Property Method Convert", "Single Class Parse", "Multiple Class Parse"]

INCLUDES = ["#include <dlfcn.h>", "#include <string_view>", "#include \"../utils/typedefs.h\"", "#include \"../utils/il2cpp-functions.h\"", "#include \"../utils/il2cpp-utils.h\""]

PARSE_HEADER = "// This .h file was compiled via beatsaber-hook/shared/helper.py's Parse Mode.\n// Created by Sc2ad.\n// Methods may not be valid!\n" + "\n".join(INCLUDES)

class writer:
    def __init__(self):
        self.arr = []
        self.prefix = ""
        self.suffix = ""
    def write(self, text):
        self.arr.append(self.prefix + text + self.suffix)
    def writeNoPrefix(self, text):
        self.arr.append(text + self.suffix)
    def indent(self):
        self.prefix += "    "
    def deindent(self):
        self.prefix = self.prefix[:len(self.prefix) - 4]
    def flush(self, dst):
        with open(dst, 'w') as f:
            f.write('\n'.join(self.arr))

def method_convert(method_lines):
    for item in method_lines:
        item = item.strip()
        spl = item.split("_")
        title = spl[0]
        method = "_".join(spl[1:])
        params = 0 if method.startswith("get") else 1
        print(item + " = il2cpp_functions::class_get_method_from_name(" + title + "_klass, \"" + method + "\", " + str(params) + ");")

def getParamTypes(method_header):
    start = method_header.index("(")
    end = method_header.index(")")
    spl = method_header[start+1:end].split(" ")
    i = 0
    while i < len(spl):
        bracket = False
        if "<" in spl[i]:
            bracket = True
        if ">" in spl[i]:
            bracket = False
        if "," in spl[i] and not bracket:
            del(spl[i])
            i -= 1
        if i == len(spl) - 1:
            # Ignore last item as well, even though it has no comma
            del(spl[i])
            i -= 1
        i += 1
    return [item for item in spl]

def class_parse(dump_data, namespace, name, dst, trace_types=False):
    dst = os.path.join(dst, namespace.replace(".", "_") + "_" + name + ".h")
    arr = []
    for i in range(len(dump_data)):
        if dump_data[i].startswith("// Namespace: " + namespace):
            # Reasonable to expect classname within 20 of i
            found = False
            for j in range(i, i + 20):
                if "class " + name + " " in dump_data[j]:
                    found = True
                    break
            if found:
                # Found matching class, let's start defining stuff!
                # 1000 lines past start of the class is reasonable
                methods_found = False
                methods = []
                for q in range(j + 2, j + 1000):
                    if dump_data[q].strip().startswith("// Methods"):
                        methods_found = True
                        continue
                    if dump_data[q].strip() == "}":
                        # Found matching class with no methods
                        if not methods_found:
                            return
                        else:
                            break
                    if methods_found and not dump_data[q].strip().startswith("["):
                        # Found a method!
                        methods.append(dump_data[q].strip())
                w = writer()
                w.write(PARSE_HEADER)
                w.write("// Contains MethodInfo/Il2CppClass data for: " + namespace + "." + name)
                klass_name = "klass"
                w.write("namespace " + namespace.replace(".", "_") + "_" + name + " {")
                w.indent()
                w.prefix += "static "
                w.suffix = ";"
                w.write("bool __cached = false")
                w.write("Il2CppClass* " + klass_name + "")
                func_lines = [klass_name + " = il2cpp_utils::GetClassFromName(\"" + namespace + "\", \"" + name + "\")"]
                w.prefix += "const MethodInfo* "
                for m in methods:
                    spl = m.split("(")[0].split(" ")
                    m_name = spl[-1]
                    m_ret = spl[len(spl) - 2]
                    # Skip .ctor/..ctor
                    if m_name.startswith("."):
                        continue
                    p = getParamTypes(m)
                    # TODO Iterate over all parameter types, find their namespaces, find their names, cache them and call GetClassFromName on them
                    # TODO Then, use these classes' type params along with type_equals (or some helper) to get the correct method
                    m_params = len(p)
                    desc_name = m_name + "_" + "_".join(p) if m_params > 0 and not m_name.startswith("set") else m_name
                    desc_name = desc_name.replace("[]", "_arr").replace("*", "_ptr").replace("`", "").replace(".", "_")
                    if m_ret == "T":
                        desc_name += "_generic"
                    generic_removal = desc_name.split("<")
                    for i in range(len(generic_removal)):
                        if ">" in generic_removal[i]:
                            generic_removal[i] = generic_removal[i][generic_removal[i].index(">") + 1:]
                    desc_name = ''.join(generic_removal)
                    func_lines.append(desc_name + " = il2cpp_functions::class_get_method_from_name(" + klass_name + ", \"" + m_name + "\", " + str(m_params) + ")")
                    w.write(desc_name)
                w.prefix = ""
                w.suffix = ""
                w.indent()
                w.write("// The Initialization function that must be called before using any of these definitions")
                w.write("static void Init() {")
                w.indent()
                w.write("if (!__cached) {")
                w.indent()
                w.suffix = ";"
                for item in func_lines:
                    w.write(item)
                w.write("__cached = true")
                w.suffix = ""
                w.deindent()
                w.write("}")
                w.deindent()
                w.write("}")
                w.deindent()
                w.write("}")

                w.flush(dst)
                return
    raise Exception("Could not find class: " + namespace + "." + name)

def parse_many(data, dat, dst):
    for item in dat:
        if type(item) == dict:
            if not 'traceUsedTypes' in item:
                item['traceUsedTypes'] = False
            class_parse(data, item['namespace'], item['class'], dst, trace_types=item['traceUsedTypes'])
        elif type(item) == list or type(item) == tuple:
            class_parse(data, item[0], item[1], dst)
        else:
            raise TypeError("dat must be list of type dict, list, or tuple!")
    

if __name__ == '__main__':
    if len(sys.argv) == 1:
        # Please enter mode:
        parse = input("Please enter mode:\n" + '\n'.join([str(i) + ": " + modes[i] for i in range(len(modes))]) + "\n")
        if parse == "0":
            arr = []
            inp = input("Enter text to convert or 'q' to finish: ")
            while inp != 'q':
                arr.append(inp)
                inp = input("Enter text to convert or 'q' to finish: ")
            method_convert(arr)
        elif parse == "1":
            dump = input("Enter path to dump.cs: ")
            with open(dump, 'r') as f:
                data = f.readlines()
            namespace = input("Enter namespace of class to convert: ")
            klass = input("Enter class name to convert: ")
            dst = input("Enter output directory (or press enter for cwd): ")
            class_parse(data, namespace, klass, dst)
        elif parse == "2":
            dump = input("Enter path to dump.cs or path to JSON: ")
            with open(dump, 'r') as f:
                if dump.endswith(".json"):
                    d = json.load(f)
                    with open(d['dump'], 'r') as q:
                        data = q.readlines()
                    dat = d['classes']
                    dst = d['dst']
                else:
                    data = f.readlines()
                    
                    dst = input("Enter output directory (or press enter for cwd): ")
                    p = input("Enter namespace (type q at any point to exit): ")
                    if os.path.exists(p):
                        with open(p, 'r') as f:
                            dat = json.load(f)
                    else:
                        dat = []
                        while p != 'q':
                            k = input("Enter class name (type q at any point to exit): ")
                            if k == 'q':
                                break
                            dat.append({
                                "namespace": p,
                                "class": k
                            })
                            p = input("Enter namespace (type q at any point to exit): ")
                parse_many(data, dat, dst)
