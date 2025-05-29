from primus.aegis.fheruntime import Py2MLIRConverter

def test_case_add(x1: float, x2: float):
    return x1 + x2

def test_case_sub(x1: float, x2: float):
    return x1 - x2

def test_case_mul(x1: float, x2: float):
    return x1 * x2

def test_case_div(x1: float, x2: float):
    return x1 / x2

def test_case_mod(x1: int, x2: int):
    return x1 % x2

def test_case_gt(x1: float, x2: float):
    return x1 > x2

def test_case_lt(x1: float, x2: float):
    return x1 < x2

def test_case_ge(x1: float, x2: float):
    return x1 >= x2

def test_case_le(x1: float, x2: float):
    return x1 <= x2

def test_case_eq(x1: float, x2: float):
    return x1 == x2

def test_case_neq(x1: float, x2: float):
    return x1 != x2

def test_case_max(x1: float, x2: float):
    x = 0
    if x1 > x2:
        x = x1
    else:
        x = x2
    return x

def test_case_min(x1: float, x2: float):
    x = 0
    if x1 < x2:
        x = x1
    else:
        x = x2
    return x

def test_case_sum(x: list[float, 10]):
    s = 0.0
    for i in range(10):
        s += x[i]
    return s

if __name__ == '__main__':
    converter = Py2MLIRConverter('./test_cases')
    converter.convert(test_case_add)
    converter.convert(test_case_sub)
    converter.convert(test_case_mul)
    converter.convert(test_case_div)
    converter.convert(test_case_mod)

    converter.convert(test_case_gt)
    converter.convert(test_case_lt)
    converter.convert(test_case_ge)
    converter.convert(test_case_le)
    converter.convert(test_case_eq)
    converter.convert(test_case_neq)

    converter.convert(test_case_max)
    converter.convert(test_case_min)

    converter.convert(test_case_sum)
