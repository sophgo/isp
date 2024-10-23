#coding:utf-8
import sys
import re
import os
headList = list()
cont = ""
elements = list()
macros_dict = {}
keys_set = set()
operators = [
    r'==',
    r'!=',
    r'>=',
    r'<=',
    r'&&',
    r'\|\|',
    r'<<',
    r'>>',
    r'>',
    r'<',
    r'\+',
    r'\-',
    r'\*',
    r'/',
    r'%',
    r'!',
    r'&',
    r'\|',
    r'\^',
    r'~',
    r'\?',
    r':',
    r'\(',
    r'\)'
]
def remove_comments(code):
    code = re.sub(r'//.*', '', code)
    code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)
    return code
def is_judge(input_string):
    return bool(re.match(r'^[-+]?((0[xX][0-9a-fA-F]+)|(0[0-7]+)|(0[bB][01]+)|([0-9]+))(\.[0-9]+)?([eE][-+]?[0-9]+)?[uUlLfFdD]*$', input_string))
def is_c_expression(express):
    operator_pattern = '|'.join(operators)
    pattern = re.compile(r'^\s*[\d\s.a-zA-Z_{}]*$'.format(operator_pattern))
    filter = r'[a-zA-Z0-9]+\('
    return bool(re.match(pattern, express)) and not bool(re.match(filter,express))
def is_varible(s):
    return bool(re.match(r'^\s*[a-zA-Z_][a-zA-Z0-9_]*\s*$',s))
def getVar(example_str):
    pattern = r'|'.join(operators)
    result = re.split(pattern, example_str)
    for r in result:
        r = r.strip()
        if len(r) == 0:
            continue
        if (is_varible(r) == False and is_judge(r) == False) or (is_varible(r) and r not in keys_set) :
            return False
        if is_varible(r) == True and r not in macros_dict:
            return True
    return None
def init():
    script_path = os.path.abspath(sys.argv[0])
    script_dir = os.path.dirname(script_path)
    os.chdir(script_dir)
    global cont
    for f in headList:
        with open(f,'r') as file:
            cont+=file.read()
            file.close()
    cont = remove_comments(cont)
def parseMacro():
    global cont
    macro_pattern = re.compile(r'#define\s+(\w+)[ \t\f\v]+(.*)')
    macros = macro_pattern.findall(cont)
    for macro in macros:
        name,value = macro
        keys_set.add(name)
    flag=True
    while flag:
        flag = False
        for macro in macros:
            name, value = macro
            pattern = r'^\((.*)\)$'
            value = re.sub(pattern, r'\1', value)
            val=value.strip()
            if is_c_expression(val)==False :
                keys_set.discard(name)
                continue
            ret=getVar(val)
            if ret==False:
                continue
            elif ret==True:
                flag=True
                print(f'val:{val}')
                continue
            macros_dict.update({name:val})
def parseElemnt(content):
    content=re.sub(r'^#.*\n', '', content, flags=re.MULTILINE)
    pattern = r'enum\s+\w+\s*{([^}]*)}'
    matches = re.findall(pattern, content, re.DOTALL)
    for m in matches:
        express="0"
        elements.clear()
        m += ','
        m = re.sub(r'\s', '', m)
        m = re.sub(r'.*[^\,]\n','',m)
        result = m.split(',')
        for i,res in enumerate(result):
            if len(res) == 0 or res[0] == '#':
                continue
            tmp = res.strip()
            if i == 0 and '=' not in tmp:
                tmp += "=0"
            elements.append(tmp)
        for element in elements:
            if '=' in element:
                name,val = element.split('=')
                element = name.strip()
                val = val.strip()
                express = val
            macros_dict.update({element:express})
            keys_set.add(element)
            express = "("+express+")"+"+1"
def write_cpp_file():
    lines=list()
    lines.append('#include <iostream>\n\n')
    lines.append('#include <sstream>\n\n')
    lines.append('int main() {\n')
    lines.append(' std::ostringstream oss;\n')
    txtLines=list()
    temp = macros_dict.copy()
    for key, value in macros_dict.items():
        lines.append(f'#define {key} {value}\n')
        try:
            value=eval(value)
            txtLines.append(f'{key}={value}\n')
        except Exception as e:
            lines.append(" oss<< \"{0}\" <<'='<<({1})<<\"\\n\";\n".format(key,value))
            pass
    lines.append('std::cout << oss.str();')
    lines.append('    return 0;\n')
    lines.append('}\n')
    with open('values.cpp', 'w', buffering=8192) as f:
        f.writelines(lines)
        f.close()
    with open('./output.txt','w') as f:
        f.writelines(txtLines)
        f.close()
    print('gengerate cpp file sucess!!')
if __name__ == '__main__':
    for idx in range(1,len(sys.argv)):
        if ".h" in sys.argv[idx]:
            headList.append(sys.argv[idx])
    init()
    parseElemnt(cont)
    parseMacro()
    write_cpp_file()
