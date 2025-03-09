#!/usr/bin/env python3
import sys, re, os

# 전역: 외부 함수 목록과 라벨 카운터
extern_functions = set()
label_counter = 0

def get_label(prefix="L"):
    global label_counter
    label = f"{prefix}{label_counter}"
    label_counter += 1
    return label

def process_libraries(source, base_path="."):
    """
    소스 파일 내에 있는 library "파일명"; 지시어를 찾아서
    해당 라이브러리 파일의 내용을 읽어온 후, 라이브러리 소스들의 리스트와
    라이브러리 지시어를 제거한 메인 소스를 반환합니다.
    """
    lib_sources = []
    new_lines = []
    for line in source.splitlines():
        m = re.match(r'library\s+"([^"]+)"\s*;', line)
        if m:
            lib_filename = m.group(1)
            lib_path = os.path.join(base_path, lib_filename)
            if not os.path.exists(lib_path):
                raise Exception("라이브러리 파일을 찾을 수 없습니다: " + lib_filename)
            with open(lib_path, "r") as f:
                lib_sources.append(f.read())
        else:
            new_lines.append(line)
    return "\n".join(new_lines), lib_sources

def parse_functions(source):
    """
    소스 코드 내 함수 정의를 파싱합니다.
    함수 정의 형식: 함수명(매개변수목록) { ... }
    반환값: {함수명: (매개변수 리스트, [본문 라인들])}
    """
    functions = {}
    lines = source.splitlines()
    current_func = None
    current_params = []
    current_body = []
    brace_count = 0
    for line in lines:
        header_match = re.match(r'\s*(\w+)\s*\((.*?)\)\s*\{', line)
        if header_match:
            current_func = header_match.group(1)
            param_str = header_match.group(2).strip()
            if param_str == "":
                current_params = []
            else:
                params = []
                for param in param_str.split(","):
                    param = param.strip()
                    param_match = re.match(r'(?:int\s+)?(\w+)', param)
                    if param_match:
                        params.append(param_match.group(1))
                current_params = params
            current_body = []
            brace_count = 1
            rest = line.split("{", 1)[1]
            if rest.strip():
                current_body.append(rest)
            continue
        if current_func is not None:
            brace_count += line.count("{")
            brace_count -= line.count("}")
            if brace_count <= 0:
                line_without_brace = line.split("}", 1)[0]
                if line_without_brace.strip():
                    current_body.append(line_without_brace)
                functions[current_func] = (current_params, current_body)
                current_func = None
                current_body = []
                current_params = []
                brace_count = 0
                continue
            current_body.append(line)
    return functions

def new_variable(local_vars, var_name, next_offset):
    if var_name not in local_vars:
        local_vars[var_name] = next_offset
        return next_offset + 8
    return next_offset

def load_operand(operand, local_vars):
    """
    operand가 숫자면 즉시값, 변수면 [rbp - offset] 형식의 메모리 주소를 반환합니다.
    """
    if operand.isdigit():
        return operand
    elif operand in local_vars:
        offset = local_vars[operand]
        return "QWORD [rbp - {}]".format(offset)
    else:
        raise Exception("정의되지 않은 변수: " + operand)

def generate_condition_code(condition, local_vars):
    """
    조건식 (예: "a == b", "i < 10" 등)을 파싱하여
    좌측 피연산자를 rax에 로드한 후 cmp 명령을 생성하고,
    조건이 거짓일 때 jump할 명령어(jne, je, jge, jle, jg, jl)를 결정합니다.
    """
    m = re.match(r'(\w+)\s*(==|!=|<|>|<=|>=)\s*(\w+)', condition)
    if m:
        left, op, right = m.groups()
        code = []
        left_op = load_operand(left, local_vars)
        code.append("    mov rax, {}".format(left_op))
        right_op = load_operand(right, local_vars)
        code.append("    cmp rax, {}".format(right_op))
        if op == "==":
            jump_instr = "jne"
        elif op == "!=":
            jump_instr = "je"
        elif op == "<":
            jump_instr = "jge"
        elif op == ">":
            jump_instr = "jle"
        elif op == "<=":
            jump_instr = "jg"
        elif op == ">=":
            jump_instr = "jl"
        else:
            raise Exception("지원하지 않는 조건 연산자: " + op)
        return code, jump_instr
    else:
        raise Exception("지원하지 않는 조건식: " + condition)

def transpile_statements(lines, local_vars, next_offset):
    """
    코드 블록(본문)을 NASM 어셈블리 코드로 변환합니다.
    지원하는 문법:
      - 변수 선언/대입 (예: int a = 10; / a = 5;)
      - 덧셈 연산 포함 변수 선언 (예: int c = a + b;)
      - if/else, while, for 문 (다양한 조건식 지원)
      - 인라인 asm{ } 블록
      - 함수 호출 (매개변수 포함)
      - return 문
    반환값: (생성된 어셈블리 코드 리스트, next_offset, 처리한 라인 수)
    """
    asm = []
    index = 0
    while index < len(lines):
        line = lines[index].strip()
        if line == "" or line.startswith("//"):
            index += 1
            continue
        if line == "}":
            return asm, next_offset, index + 1
        # 인라인 어셈블리 블록
        if line.startswith("asm{"):
            asm_block = []
            if "}" in line:
                content = line[line.find("asm{")+len("asm{"):line.find("}")]
                asm_block.append(content.strip())
                for l in asm_block:
                    asm.append("    " + l)
                index += 1
                continue
            else:
                index += 1
                while index < len(lines) and "}" not in lines[index]:
                    asm_block.append(lines[index].rstrip())
                    index += 1
                if index < len(lines):
                    content = lines[index].split("}")[0].strip()
                    if content:
                        asm_block.append(content)
                    index += 1
                for l in asm_block:
                    asm.append("    " + l)
                continue
        # if 문: if (condition) { ... } [else { ... }]
        m_if = re.match(r'if\s*\((.*?)\)\s*\{', line)
        if m_if:
            condition = m_if.group(1)
            cond_code, jump_instr = generate_condition_code(condition, local_vars)
            else_label = get_label("ELSE")
            end_label = get_label("ENDIF")
            asm.extend(cond_code)
            asm.append("    {} {}".format(jump_instr, else_label))
            if_block, next_offset, new_index = transpile_statements(lines[index+1:], local_vars.copy(), next_offset)
            asm.extend(if_block)
            asm.append("    jmp " + end_label)
            asm.append(else_label + ":")
            remaining = lines[index+1:][new_index:]
            if remaining and re.match(r'else\s*\{', remaining[0].strip()):
                else_block, next_offset, new_index2 = transpile_statements(remaining[1:], local_vars.copy(), next_offset)
                asm.extend(else_block)
                new_index += 1 + new_index2
            asm.append(end_label + ":")
            index = index + 1 + new_index
            continue
        # while 문: while (condition) { ... }
        m_while = re.match(r'while\s*\((.*?)\)\s*\{', line)
        if m_while:
            condition = m_while.group(1)
            start_label = get_label("LOOP_START")
            end_label = get_label("LOOP_END")
            asm.append(start_label + ":")
            cond_code, jump_instr = generate_condition_code(condition, local_vars)
            asm.extend(cond_code)
            asm.append("    {} {}".format(jump_instr, end_label))
            loop_block, next_offset, new_index = transpile_statements(lines[index+1:], local_vars.copy(), next_offset)
            asm.extend(loop_block)
            asm.append("    jmp " + start_label)
            asm.append(end_label + ":")
            index = index + 1 + new_index
            continue
        # for 문: for (init; condition; post) { ... }
        m_for = re.match(r'for\s*\((.*?);(.*?);(.*?)\)\s*\{', line)
        if m_for:
            init_stmt = m_for.group(1).strip()
            condition = m_for.group(2).strip()
            post_stmt = m_for.group(3).strip()
            init_lines = [init_stmt + ";"]
            init_code, next_offset, _ = transpile_statements(init_lines, local_vars, next_offset)
            asm.extend(init_code)
            start_label = get_label("FOR_START")
            end_label = get_label("FOR_END")
            asm.append(start_label + ":")
            cond_code, jump_instr = generate_condition_code(condition, local_vars)
            asm.extend(cond_code)
            asm.append("    {} {}".format(jump_instr, end_label))
            loop_block, next_offset, new_index = transpile_statements(lines[index+1:], local_vars.copy(), next_offset)
            asm.extend(loop_block)
            post_lines = [post_stmt + ";"]
            post_code, next_offset, _ = transpile_statements(post_lines, local_vars, next_offset)
            asm.extend(post_code)
            asm.append("    jmp " + start_label)
            asm.append(end_label + ":")
            index = index + 1 + new_index
            continue
        # 변수 선언: int a = 10;
        m_decl = re.match(r'int\s+(\w+)\s*=\s*(\d+)\s*;', line)
        if m_decl:
            var, value = m_decl.groups()
            next_offset = new_variable(local_vars, var, next_offset)
            asm.append("    mov rax, {}".format(value))
            asm.append("    mov QWORD [rbp - {}], rax".format(local_vars[var]))
            index += 1
            continue
        # 변수 선언 및 덧셈: int c = a + b;
        m_add = re.match(r'int\s+(\w+)\s*=\s*(\w+)\s*\+\s*(\w+)\s*;', line)
        if m_add:
            var, op1, op2 = m_add.groups()
            next_offset = new_variable(local_vars, var, next_offset)
            asm.append("    mov rax, {}".format(load_operand(op1, local_vars)))
            asm.append("    add rax, {}".format(load_operand(op2, local_vars)))
            asm.append("    mov QWORD [rbp - {}], rax".format(local_vars[var]))
            index += 1
            continue
        # 단순 대입: a = 5;
        m_assign = re.match(r'(\w+)\s*=\s*(\d+)\s*;', line)
        if m_assign:
            var, value = m_assign.groups()
            if var not in local_vars:
                raise Exception("정의되지 않은 변수: " + var)
            asm.append("    mov rax, {}".format(value))
            asm.append("    mov QWORD [rbp - {}], rax".format(local_vars[var]))
            index += 1
            continue
        # 함수 호출 (매개변수 포함): foo(a, 10);
        m_call_args = re.match(r'(\w+)\s*\((.*?)\)\s*;', line)
        if m_call_args:
            func_name = m_call_args.group(1)
            args_str = m_call_args.group(2).strip()
            args = []
            if args_str != "":
                args = [arg.strip() for arg in args_str.split(",")]
            param_regs = ["rdi", "rsi", "rdx", "rcx", "r8", "r9"]
            for i, arg in enumerate(args):
                if i >= len(param_regs):
                    raise Exception("매개변수 개수 초과: " + line)
                operand = load_operand(arg, local_vars)
                asm.append("    mov {}, {}".format(param_regs[i], operand))
            if func_name not in defined_functions:
                extern_functions.add(func_name)
            asm.append("    call " + func_name)
            index += 1
            continue
        # 함수 호출 (매개변수 없는 경우): foo();
        m_call = re.match(r'(\w+)\s*\(\s*\)\s*;', line)
        if m_call:
            func_name = m_call.group(1)
            if func_name not in defined_functions:
                extern_functions.add(func_name)
            asm.append("    call " + func_name)
            index += 1
            continue
        # return 문: return a;
        m_return = re.match(r'return\s+(\w+)\s*;', line)
        if m_return:
            var = m_return.group(1)
            asm.append("    mov rax, {}".format(load_operand(var, local_vars)))
            asm.append("    jmp .Lreturn")
            index += 1
            continue
        index += 1
    return asm, next_offset, index

# 전역 함수 집합
defined_functions = set()

def transpile_function(func_name, params, body_lines):
    """
    함수 프로토타입과 본문을 받아 스택 프레임(프로로그/에필로그)을 구성합니다.
    커널 코드로 사용하기 위해 매개변수 전달과 로컬 변수 할당을 처리합니다.
    """
    local_vars = {}
    next_offset = 0
    param_regs = ["rdi", "rsi", "rdx", "rcx", "r8", "r9"]
    for i, param in enumerate(params):
        next_offset = new_variable(local_vars, param, next_offset)
    body_asm, next_offset, _ = transpile_statements(body_lines, local_vars, next_offset)
    frame_size = next_offset
    prologue = []
    prologue.append("push rbp")
    prologue.append("mov rbp, rsp")
    if frame_size > 0:
        prologue.append("sub rsp, {}".format(frame_size))
    for i, param in enumerate(params):
        if i < len(param_regs):
            offset = local_vars[param]
            prologue.append("mov QWORD [rbp - {}], {}".format(offset, param_regs[i]))
    epilogue = []
    epilogue.append(".Lreturn:")
    epilogue.append("mov rsp, rbp")
    epilogue.append("pop rbp")
    epilogue.append("ret")
    return prologue + body_asm + epilogue

def transpile(source, base_path="."):
    """
    전체 소스 코드를 분석하여 각 함수별 NASM 어셈블리 코드를 생성합니다.
    라이브러리 지시어(library "파일명";)를 처리하여 해당 라이브러리 파일의 소스를 메인 소스와 병합합니다.
    부트로더는 별개로 처리하며, 커널 엔트리점은 kernel_main() 또는 main() 함수가 전역 심볼로 출력됩니다.
    """
    global defined_functions
    # 라이브러리 지시어 처리
    main_source, lib_sources = process_libraries(source, base_path)
    # 라이브러리 소스와 메인 소스를 병합합니다.
    full_source = "\n".join(lib_sources) + "\n" + main_source
    functions = parse_functions(full_source)
    defined_functions = set(functions.keys())
    asm = []
    asm.append("section .text")
    if extern_functions:
        for ext in extern_functions:
            asm.insert(0, "extern " + ext)
    function_asm = []
    for func_name, (params, body_lines) in functions.items():
        function_asm.append(func_name + ":")
        function_asm.extend(transpile_function(func_name, params, body_lines))
    final_asm = []
    if "kernel_main" in defined_functions:
        final_asm.append("global kernel_main")
    elif "main" in defined_functions:
        final_asm.append("global main")
    final_asm.extend(asm)
    final_asm.extend(function_asm)
    return "\n".join(final_asm)

def main():
    if len(sys.argv) < 2:
        print("Usage: {} <filename.cslash>".format(sys.argv[0]))
        sys.exit(1)
    base_path = os.path.dirname(os.path.abspath(sys.argv[1]))
    filename = sys.argv[1]
    with open(filename, "r") as f:
        source = f.read()
    asm_code = transpile(source, base_path)
    asm_filename = filename.rsplit('.', 1)[0] + ".asm"
    with open(asm_filename, "w") as f:
        f.write(asm_code)
    print("Transpiled to:", asm_filename)

if __name__ == "__main__":
    main()
