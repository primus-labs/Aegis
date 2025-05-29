import ast
from mlir import ir
from mlir.dialects import func, arith, affine, memref, scf
from mlir.ir import ShapedType
import numpy as np

class MLIRGenerator(ast.NodeVisitor):
    """
    Visitor class that generates MLIR code from a Python AST.
    """
    def __init__(self, module):
        self.module = module
        self.symbol_table = {}
        self.block_stack = []
        self.func_table = {}
        self.variable_versions = {}
        self.expression_cache = {}
    
    def node_to_tuple(self, node):
        if isinstance(node, ast.AST):
            fields = tuple((field, self.node_to_tuple(getattr(node, field))) for field in node._fields)
            return (type(node).__name__, fields)
        elif isinstance(node, list):
            return tuple(self.node_to_tuple(item) for item in node)
        else:
            return node
        
    def get_expression_cache_key(self, node):
        node_key = self.node_to_tuple(node)
        variables_in_expr = self.collect_variables(node)
        variable_versions = tuple((var, self.variable_versions.get(var, 0)) for var in sorted(variables_in_expr))
        return (node_key, variable_versions)
    
    def get_subscript_variable_name(self, target):
        base_node = target
        while isinstance(base_node, ast.Subscript):
            base_node = base_node.value
        if isinstance(base_node, ast.Name):
            return base_node.id
        else:
            raise NotImplementedError(f"Unsupported subscript target: {ast.dump(target)}")
    
    def collect_variables(self, node):
        variables = set()
        class VariableCollector(ast.NodeVisitor):
            def visit_Name(self, n):
                if isinstance(n.ctx, ast.Load):
                    variables.add(n.id)
            def generic_visit(self, n):
                super().generic_visit(n)
        VariableCollector().visit(node)
        return variables
        
    def visit(self, node):
        cache_key = self.get_expression_cache_key(node)
        if cache_key in self.expression_cache:
            return self.expression_cache[cache_key]
        else:
            result = super().visit(node)
            self.expression_cache[cache_key] = result
            return result

    def flatten_list(self, elts):
        if all(isinstance(e, ast.Constant) and isinstance(e.value, (int, float)) for e in elts):
            values = [e.value for e in elts]
            shape = [len(elts)]
            return values, shape
        elif all(isinstance(e, ast.List) for e in elts):
            values_list = []
            shapes = []
            for e in elts:
                values, shape = self.flatten_list(e.elts)
                values_list.extend(values)
                shapes.append(shape)
            if not all(s == shapes[0] for s in shapes):
                raise ValueError("List sublists have inconsistent shapes")
            shape = [len(elts)] + shapes[0]
            return values_list, shape
        else:
            raise NotImplementedError("Unsupported list element type")

    def cast_to_index(self, value):
        index_type = ir.IndexType.get()
        with ir.InsertionPoint(self.block_stack[-1]):
            if value.type == index_type:
                return value
            elif isinstance(value.type, ir.IntegerType):
                # return arith.IndexCastOp(index_type, value).result
                pass
            elif isinstance(value.type, ir.FloatType):
                int_value = arith.FPToSIOp(ir.IntegerType.get_signless(32), value).result
                return arith.IndexCastOp(index_type, int_value).result
            else:
                raise TypeError(f"Cannot cast type {value.type} to index")

    def cast_to_int64(self, value):
        int64_type = ir.IntegerType.get_signless(32)
        with ir.InsertionPoint(self.block_stack[-1]):
            if value.type == int64_type:
                return value
            elif isinstance(value.type, ir.IndexType):
                return arith.IndexCastOp(int64_type, value).result
            elif isinstance(value.type, ir.IntegerType):
                return arith.ExtSIOp(int64_type, value).result
            elif isinstance(value.type, ir.F32Type):
                return arith.FPToSIOp(int64_type, value).result
            else:
                raise TypeError(f"Cannot cast type {value.type} to int64")

    def cast_to_f64(self, value):
        f64_type = ir.F32Type.get()
        with ir.InsertionPoint(self.block_stack[-1]):
            if value.type == f64_type:
                return value
            elif isinstance(value.type, ir.IndexType):
                int_value = self.cast_to_int64(value)
                return arith.SIToFPOp(f64_type, int_value).result
            elif isinstance(value.type, ir.IntegerType):
                return arith.SIToFPOp(f64_type, value).result
            elif isinstance(value.type, ir.F32Type):
                return arith.ExtFOp(f64_type, value).result
            else:
                raise TypeError(f"Cannot cast type {value.type} to f64")

    def parse_type_annotation(self, annotation):
        if isinstance(annotation, ast.Name):
            if annotation.id == 'float':
                return ir.F32Type.get()
            elif annotation.id == 'int':
                return ir.IntegerType.get_signless(32)
            else:
                raise NotImplementedError(f"Unsupported type annotation: {annotation.id}")
        elif isinstance(annotation, ast.Subscript):
            if (isinstance(annotation.value, ast.Name) and annotation.value.id == 'list'):
                element_type = self.parse_type_annotation(annotation.slice)
                dynamic_size = ir.ShapedType.get_dynamic_size()
                if isinstance(annotation.slice, ast.Tuple):
                    dynamic_size = annotation.slice.elts[1].value
                if isinstance(element_type, ir.MemRefType):
                    memref_type = ir.MemRefType.get([dynamic_size] + list(element_type.shape), element_type.element_type)
                elif isinstance(element_type, ir.Type):
                    memref_type = ir.MemRefType.get([dynamic_size], element_type)
                else:
                    raise NotImplementedError(f"Unsupported element type: {element_type}")
                return memref_type
            else:
                raise NotImplementedError(f"Unsupported type annotation: {ast.dump(annotation)}")
        elif isinstance(annotation, ast.Tuple):
            if isinstance(annotation.elts[0], ast.Name):
                if annotation.elts[0].id == 'float':
                    return ir.F32Type.get()
                elif annotation.elts[0].id == 'int':
                    # return ir.IntegerType.get_signless(32)
                    return ir.F32Type.get()
                else:
                    raise NotImplementedError(f"Unsupported type annotation: {annotation.id}")
            else:
                return self.parse_type_annotation(annotation.elts[0])

        else:
            raise NotImplementedError(f"Unsupported type annotation: {ast.dump(annotation)}")

    def create_subview(self, base, indices):
        base_type = base.type
        memref_shape = base_type.shape
        memref_rank = len(memref_shape)
        index_type = ir.IndexType.get()
        dynamic = ShapedType.get_dynamic_stride_or_offset()
        offsets = []
        sizes = []
        strides = []
        static_offsets = []
        static_sizes = []
        static_strides = []
        zero = arith.ConstantOp(index_type, 0).result
        one = arith.ConstantOp(index_type, 1).result
        for i in range(memref_rank):
            if i < len(indices):
                offsets.append(indices[i])
                static_offsets.append(dynamic) 
                sizes.append(one) 
                static_sizes.append(1) 
            else:
                offsets.append(zero)
                static_offsets.append(0)
                if memref_shape[i] == -1:
                    dim_size = memref.DimOp(base, i).result
                    sizes.append(dim_size)
                    static_sizes.append(dynamic) 
                else:
                    size = memref_shape[i]
                    size_value = arith.ConstantOp(index_type, size).result
                    sizes.append(size_value)
                    static_sizes.append(size)
            strides.append(one)
            static_strides.append(1)
        subview_shape = []
        for size in static_sizes:
            if size == dynamic:
                subview_shape.append(-1)
            else:
                subview_shape.append(size)
        subview_type = ir.MemRefType.get(
            subview_shape,
            base_type.element_type,
            memory_space=base_type.memory_space
        )
        subview_op = memref.SubViewOp(
            subview_type,
            base,
            offsets,
            sizes,
            strides,
            static_offsets=static_offsets,
            static_sizes=static_sizes,
            static_strides=static_strides
        )
        return subview_op


    def store_to_subscript(self, target, value):
        indices = []
        base_node = target
        while isinstance(base_node, ast.Subscript):
            index_node = base_node.slice
            if isinstance(index_node, ast.Index):
                expr = index_node.value
            else:
                expr = index_node
            indices.insert(0, expr)
            base_node = base_node.value

        base = self.visit(base_node)
        base_type = base.type
        if not isinstance(base_type, ir.MemRefType):
            raise TypeError(f"Expected a memref type, got {base_type}")
        memref_shape = base_type.shape
        memref_rank = len(memref_shape)

        if len(indices) > memref_rank:
            raise IndexError("Too many indices for memref")

        dim_vars = []
        sym_vars = []

        def collect_variable_names(expr):
            variable_names = set()

            class NameCollector(ast.NodeVisitor):
                def visit_Name(self, node):
                    variable_names.add(node.id)
                def generic_visit(self, node):
                    ast.NodeVisitor.generic_visit(self, node)

            NameCollector().visit(expr)
            return variable_names

        if len(indices) == 1:
            dim_vars = list(collect_variable_names(indices[0]))
        elif len(indices) == 2:
            dim_vars = collect_variable_names(indices[0])
            dim_vars |= collect_variable_names(indices[1])
            if dim_vars == {}:
                dim_vars =[]
            else:
                dim_vars = list(dim_vars)

        index_exprs = []

        for expr in indices:
            if isinstance(expr, ast.Name):
                var_name = expr.id
                if var_name in dim_vars:
                    dim_pos = dim_vars.index(var_name)
                    affine_expr = affine.AffineDimExpr.get(dim_pos)
                else:
                    if var_name not in sym_vars:
                        sym_vars.append(var_name)
                    sym_pos = sym_vars.index(var_name)
                    affine_expr = affine.AffineSymbolExpr.get(sym_pos)
                index_exprs.append(affine_expr)
            elif isinstance(expr, ast.Constant):
                value1 = expr.value
                affine_expr = affine.AffineConstantExpr.get(value1)
                index_exprs.append(affine_expr)
            else:
                vars_in_expr = collect_variable_names(expr)
                for var in vars_in_expr:
                    if var not in dim_vars and var not in sym_vars:
                        sym_vars.append(var)
                affine_expr, _ = self.parse_affine_expr(expr, dim_vars, sym_vars)
                index_exprs.append(affine_expr)

        num_dims = len(dim_vars)
        num_syms = len(sym_vars)
        affine_map = affine.AffineMap.get(num_dims, num_syms, index_exprs)

        map_operands = []
        for var_name in dim_vars:
            operand = self.symbol_table.get(var_name)
            if operand is None:
                for block in reversed(self.block_stack):
                    if hasattr(block, 'arguments'):
                        for arg in block.arguments:
                            arg_name = self.get_arg_name(arg)
                            if arg_name == var_name:
                                operand = arg
                                break
                    if operand:
                        break
            if operand is None:
                raise ValueError(f"Dimension variable '{var_name}' not found")
            map_operands.append(operand)
        for var_name in sym_vars:
            operand = self.symbol_table.get(var_name)
            if operand is None:
                operand = self.visit(ast.Name(id=var_name, ctx=ast.Load()))
            if operand is None:
                raise ValueError(f"Symbol variable '{var_name}' not found")
            map_operands.append(operand)
        with ir.InsertionPoint(self.block_stack[-1]):
            if base_type.element_type == ir.F32Type.get():
                value = self.cast_to_f64(value)
            elif base_type.element_type == ir.IntegerType.get_signless(32):
                # value = self.cast_to_int64(value)
                value = self.cast_to_f64(value)
            affine.AffineStoreOp(value, base, map_operands, map=affine_map)

    def visit_Module(self, node):
        self.block_stack.append(self.module.body)
        for stmt in node.body:
            self.visit(stmt)
        self.block_stack.pop()

    def visit_FunctionDef(self, node):
        arg_types = []
        for arg in node.args.args:
            if arg.annotation:
                arg_type = self.parse_type_annotation(arg.annotation)
            else:
                arg_type = ir.F32Type.get()
            arg_types.append(arg_type)
        self.return_values = []

        placeholder_func_type = ir.FunctionType.get(arg_types, [])
        with ir.InsertionPoint(self.module.body):
            func_op = func.FuncOp(name=node.name, type=placeholder_func_type)
            entry_block = func_op.add_entry_block()
            self.block_stack.append(entry_block)

            with ir.InsertionPoint(entry_block):
                self.symbol_table = {}
                for i, arg in enumerate(node.args.args):
                    arg_name = arg.arg
                    mlir_arg = entry_block.arguments[i]
                    self.symbol_table[arg_name] = mlir_arg

                for stmt in node.body:
                    self.visit(stmt)

                if self.return_values:
                    return_types = [value.type for value in self.return_values]
                else:
                    return_types = []
                func_type = ir.FunctionType.get(arg_types, return_types)
                func_op.attributes["function_type"] = ir.TypeAttr.get(func_type)
                self.func_table[node.name] = func_type

            self.block_stack.pop()


    def get_constant_int_value(self, value):
        if isinstance(value, arith.ConstantOp) or isinstance(value, ast.Constant):
            return int(value.value)
        elif isinstance(value, ast.Name):
            value = self.symbol_table.get(value.id)
            if isinstance(value, ir.OpResult) and isinstance(value.owner.opview, arith.ConstantOp):
                return int(value.owner.opview.attributes["value"].value)
            else:
                value = self.cast_to_index(value)
                return value
        else:
            value = self.cast_to_index(value)
            return value




    def parse_affine_expr(self, expr, dim_vars, sym_vars):
        if isinstance(expr, ast.Name):
            var_name = expr.id
            varop = self.symbol_table.get(var_name)
            if isinstance(varop, ir.OpResult):
                if isinstance(varop.owner.opview, arith.ConstantOp):
                    varvalue = int(varop.owner.opview.literal_value)
                    dim_vars.remove(var_name)
                    return affine.AffineConstantExpr.get(varvalue), []
            if var_name in dim_vars:
                dim_pos = dim_vars.index(var_name)
                return affine.AffineDimExpr.get(dim_pos), []
            else:
                if var_name not in sym_vars:
                    sym_vars.append(var_name)
                sym_pos = sym_vars.index(var_name)
                return affine.AffineSymbolExpr.get(sym_pos), []
        elif isinstance(expr, ast.Constant):
            value = expr.value
            return affine.AffineConstantExpr.get(value), []
        elif isinstance(expr, ast.BinOp):
            lhs_expr, lhs_operands = self.parse_affine_expr(expr.left, dim_vars, sym_vars)
            rhs_expr, rhs_operands = self.parse_affine_expr(expr.right, dim_vars, sym_vars)
            if isinstance(expr.op, ast.Add):
                return lhs_expr + rhs_expr, lhs_operands + rhs_operands
            elif isinstance(expr.op, ast.Sub):
                return lhs_expr - rhs_expr, lhs_operands + rhs_operands
            elif isinstance(expr.op, ast.Mult):
                return lhs_expr * rhs_expr, lhs_operands + rhs_operands
            elif isinstance(expr.op, ast.Div):
                return lhs_expr / rhs_expr, lhs_operands + rhs_operands
            elif isinstance(expr.op, ast.Mod):
                return lhs_expr % rhs_expr, lhs_operands + rhs_operands
            else:
                raise NotImplementedError(f"Unsupported binary operator in affine expression: {type(expr.op)}")
        else:
            raise NotImplementedError(f"Unsupported expression in affine index: {ast.dump(expr)}")

    def visit_Subscript(self, node):
        indices = []
        base_node = node
        while isinstance(base_node, ast.Subscript):
            index_node = base_node.slice
            if isinstance(index_node, ast.Index):
                expr = index_node.value
            else:
                expr = index_node
            indices.insert(0, expr) 
            base_node = base_node.value

        base = self.visit(base_node)
        base_type = base.type
        if not isinstance(base_type, ir.MemRefType):
            raise TypeError(f"Expected a memref type, got {base_type}")
        memref_shape = base_type.shape
        memref_rank = len(memref_shape)

        if len(indices) > memref_rank:
            raise IndexError("Too many indices for memref")
        dim_vars = []
        sym_vars = []

        def collect_variable_names(expr):
            variable_names = []
            class NameCollector(ast.NodeVisitor):
                def visit_Name(self, node):
                    if node.id not in variable_names:
                        variable_names.append(node.id)
                def generic_visit(self, node):
                    ast.NodeVisitor.generic_visit(self, node)
            NameCollector().visit(expr)
            return variable_names
        if len(indices) == 1:
            dim_vars = collect_variable_names(indices[0])
        elif len(indices) == 2:
            dim_vars = collect_variable_names(indices[0])
            dim_vars2 = collect_variable_names(indices[1])
            for i in dim_vars2:
                if i not in dim_vars:
                    dim_vars.append(i)
        index_exprs = []
        for expr in indices:
            if isinstance(expr, ast.Name):
                var_name = expr.id
                if var_name in dim_vars:
                    dim_pos = dim_vars.index(var_name)
                    affine_expr = affine.AffineDimExpr.get(dim_pos)
                else:
                    if var_name not in sym_vars:
                        sym_vars.append(var_name)
                    sym_pos = sym_vars.index(var_name)
                    affine_expr = affine.AffineSymbolExpr.get(sym_pos)
                index_exprs.append(affine_expr)
            elif isinstance(expr, ast.Constant):
                value1 = expr.value
                affine_expr = affine.AffineConstantExpr.get(value1)
                index_exprs.append(affine_expr)
            else:
                vars_in_expr = collect_variable_names(expr)
                for var in vars_in_expr:
                    if var not in dim_vars and var not in sym_vars:
                        sym_vars.append(var)
                affine_expr, _ = self.parse_affine_expr(expr, dim_vars, sym_vars)
                index_exprs.append(affine_expr)
        num_dims = len(dim_vars)
        num_syms = len(sym_vars)
        affine_map = affine.AffineMap.get(num_dims, num_syms, index_exprs)

        map_operands = []
        for var_name in dim_vars:
            operand = self.symbol_table.get(var_name)
            if operand is None:
                for block in reversed(self.block_stack):
                    if hasattr(block, 'arguments'):
                        for arg in block.arguments:
                            arg_name = self.get_arg_name(arg)
                            if arg_name == var_name:
                                operand = arg
                                break
                    if operand:
                        break
            if operand is None:
                raise ValueError(f"Dimension variable '{var_name}' not found")
            if isinstance(operand, ir.OpResult):
                if isinstance(operand.owner.opview, arith.ConstantOp):
                    operand = operand.owner.opview
                else:
                    operand = self.cast_to_index(operand)
            map_operands.append(operand)
        for var_name in sym_vars:
            operand = self.symbol_table.get(var_name)
            if operand is None:
                operand = self.visit(ast.Name(id=var_name, ctx=ast.Load()))
            if operand is None:
                raise ValueError(f"Symbol variable '{var_name}' not found")
            map_operands.append(operand)

        with ir.InsertionPoint(self.block_stack[-1]):
            load_op = affine.AffineLoadOp(base_type.element_type, base, map=affine_map, indices = map_operands)
            return load_op.result

    def get_arg_name(self, arg):
        for name, value in self.symbol_table.items():
            if value == arg:
                return name
        return None

    def visit_List(self, node):
        values, shape = self.flatten_list(node.elts)
        if all(isinstance(v, int) for v in values):
            # element_type = ir.IntegerType.get_signless(32)
            element_type = ir.F32Type.get()
            eltype = 0
        else:
            element_type = ir.F32Type.get()
            eltype = 1
        memref_type = ir.MemRefType.get(shape, element_type)
        dynamic_sizes = []
        symbol_operands = []
        with ir.InsertionPoint(self.block_stack[-1]):
            memref_alloc = memref.AllocOp(memref_type, dynamic_sizes, symbol_operands).result
            if eltype == 0:
                np_values = np.array(values, dtype=np.int32).reshape(shape)
            else:
                np_values = np.array(values, dtype=np.float32).reshape(shape)
            for idx, value in np.ndenumerate(np_values):
                if len(idx) == 1:
                    map = affine.AffineMap.get_constant(idx[0])
                    affine.AffineStoreOp(arith.ConstantOp(element_type, value.item()), memref_alloc, map = map, indices = [])
                elif len(idx) == 2:
                    map = affine.AffineMap.get(0, 0, [affine.AffineConstantExpr.get(idx[0]), affine.AffineConstantExpr.get(idx[1])])
                    affine.AffineStoreOp(arith.ConstantOp(element_type, value.item()), memref_alloc, map = map, indices = [])
        return memref_alloc

    def visit_Call(self, node):
        func_name = node.func.id
        if func_name not in self.func_table:
            raise NameError(f"Function '{func_name}' is not defined.")
        func_type = self.func_table[func_name]
        return_types = func_type.results
        args = [self.visit(arg) for arg in node.args]
        with ir.InsertionPoint(self.block_stack[-1]):
            call_op = ir.Operation.create(
                "func.call",
                results=return_types,
                operands=args,
                attributes={"callee": ir.FlatSymbolRefAttr.get(func_name)}
            )
        if len(return_types) == 0:
            return None
        elif len(return_types) == 1:
            return call_op.result
        else:
            return tuple(call_op.results)

    def visit_Return(self, node):
        return_value = self.visit(node.value)
        with ir.InsertionPoint(self.block_stack[-1]):
            func.ReturnOp([return_value])
        self.return_values.append(return_value)

    def visit_Name(self, node):
        value = self.symbol_table.get(node.id)
        if value is None:
            raise NameError(f"Value '{node.id}' is not defined")
        return value

    def visit_BinOp(self, node):
        left = self.visit(node.left)
        right = self.visit(node.right)
        if isinstance(left.type, ir.F32Type) or isinstance(right.type, ir.F32Type):
            left = self.cast_to_f64(left)
            right = self.cast_to_f64(right)
            with ir.InsertionPoint(self.block_stack[-1]):
                if isinstance(node.op, ast.Add):
                    op = arith.AddFOp(left, right)
                elif isinstance(node.op, ast.Sub):
                    op = arith.SubFOp(left, right)
                elif isinstance(node.op, ast.Mult):
                    op = arith.MulFOp(left, right)
                elif isinstance(node.op, ast.Div):
                    op = arith.DivFOp(left, right)
                elif isinstance(node.op, ast.Mod):
                    op = arith.RemFOp(left, right)
                else:
                    raise NotImplementedError(f"Unsupported operator: {type(node.op)}")
        elif isinstance(left.type, (ir.IntegerType, ir.IndexType)) and isinstance(right.type, (ir.IntegerType, ir.IndexType)):
            left = self.cast_to_int64(left)
            right = self.cast_to_int64(right)
            with ir.InsertionPoint(self.block_stack[-1]):
                if isinstance(node.op, ast.Add):
                    op = arith.AddIOp(left, right)
                elif isinstance(node.op, ast.Sub):
                    op = arith.SubIOp(left, right)
                elif isinstance(node.op, ast.Mult):
                    op = arith.MulIOp(left, right)
                elif isinstance(node.op, ast.Div):
                    op = arith.DivSIOp(left, right)
                elif isinstance(node.op, ast.Mod):
                    op = arith.RemSIOp(left, right)
                else:
                    raise NotImplementedError(f"Unsupported operator: {type(node.op)}")
        else:
            raise TypeError("Operand types do not match for binary operation.")

        return op.result

    def visit_Compare(self, node):
        left = self.visit(node.left)
        right = self.visit(node.comparators[0])
        left = self.cast_to_f64(left)
        right = self.cast_to_f64(right)
        with ir.InsertionPoint(self.block_stack[-1]):
            if isinstance(node.ops[0], ast.Gt):
                op = arith.CmpFOp(arith.CmpFPredicate.OGT, left, right)
            elif isinstance(node.ops[0], ast.Lt):
                op = arith.CmpFOp(arith.CmpFPredicate.OLT, left, right)
            elif isinstance(node.ops[0], ast.Eq):
                op = arith.CmpFOp(arith.CmpFPredicate.OEQ, left, right)
            elif isinstance(node.ops[0], ast.GtE):
                op = arith.CmpFOp(arith.CmpFPredicate.OGE, left, right)
            elif isinstance(node.ops[0], ast.LtE):
                op = arith.CmpFOp(arith.CmpFPredicate.OLE, left, right)
            elif isinstance(node.ops[0], ast.NotEq):
                op = arith.CmpFOp(arith.CmpFPredicate.ONE, left, right)
            else:
                raise NotImplementedError(f"Unsupported comparison operator: {type(node.ops[0])}")
        return op.result
    
    def visit_If(self, node):
        # Visit the condition
        cond_value = self.visit(node.test)
        # Collect variables assigned in 'then' and 'else' blocks
        def collect_assigned_vars(stmts):
            assigned_vars = set()
            for stmt in stmts:
                if isinstance(stmt, (ast.Assign, ast.AugAssign)):
                    target = stmt.targets[0] if isinstance(stmt, ast.Assign) else stmt.target
                    if isinstance(target, ast.Name):
                        assigned_vars.add(target.id)
                elif isinstance(stmt, ast.If):
                    assigned_vars.update(collect_assigned_vars(stmt.body))
                    assigned_vars.update(collect_assigned_vars(stmt.orelse))
                elif isinstance(stmt, ast.For):
                    assigned_vars.update(collect_assigned_vars(stmt.body))
            return assigned_vars
        then_assigned_vars = collect_assigned_vars(node.body)
        else_assigned_vars = collect_assigned_vars(node.orelse)
        assigned_vars = then_assigned_vars.union(else_assigned_vars)
        symbol_table_before_if = {}
        result_types = []
        for var_name in assigned_vars:
            var_value = self.symbol_table.get(var_name)
            if var_value is None:
                raise NameError(f"Variable '{var_name}' assigned in 'if' block is not defined before")
            symbol_table_before_if[var_name] = var_value
            result_types.append(var_value.type)
        has_else = (len(node.orelse) > 0) or (len(assigned_vars) > 0)
        with ir.InsertionPoint(self.block_stack[-1]):
            if_op = scf.IfOp(cond_value, result_types, hasElse=has_else)
        # 'then' block
        self.block_stack.append(if_op.then_block)
        with ir.InsertionPoint(if_op.then_block):
            for stmt in node.body:
                self.visit(stmt)
            # Collect the values of assigned variables
            yield_values = []
            for var_name in assigned_vars:
                value = self.symbol_table.get(var_name)
                if var_name not in then_assigned_vars:
                    # Variable not assigned in 'then' block, use initial value
                    value = symbol_table_before_if[var_name]
                yield_values.append(value)
            scf.YieldOp(yield_values)
        self.block_stack.pop()
        # 'else' block
        if node.orelse:
            self.block_stack.append(if_op.else_block)
            with ir.InsertionPoint(if_op.else_block):
                for stmt in node.orelse:
                    self.visit(stmt)
                # Collect the values of assigned variables
                yield_values = []
                for var_name in assigned_vars:
                    value = self.symbol_table.get(var_name)
                    if var_name not in else_assigned_vars:
                        # Variable not assigned in 'else' block, use initial value
                        value = symbol_table_before_if[var_name]
                    yield_values.append(value)
                scf.YieldOp(yield_values)
            self.block_stack.pop()
        elif has_else:
            # Create 'else' block that yields initial values
            self.block_stack.append(if_op.else_block)
            with ir.InsertionPoint(if_op.else_block):
                yield_values = [symbol_table_before_if[var_name] for var_name in assigned_vars]
                scf.YieldOp(yield_values)
            self.block_stack.pop()
        # After the 'if_op', update symbol_table
        for idx, var_name in enumerate(assigned_vars):
            self.symbol_table[var_name] = if_op.results[idx]

    def visit_For(self, node):
        if not (isinstance(node.iter, ast.Call) and
                isinstance(node.iter.func, ast.Name) and
                node.iter.func.id == 'range'):
            raise NotImplementedError("Only loops of the form 'for ... in range(...)' are supported, where range can contain variables")
        range_args = node.iter.args
        with ir.InsertionPoint(self.block_stack[-1]):
            if len(range_args) == 1:
                start = 0
                # end_value = self.visit(range_args[0])
                end = self.get_constant_int_value(range_args[0])
                step = 1
            elif len(range_args) == 2:
                # start_value = self.visit(range_args[0])
                start = self.get_constant_int_value(range_args[0])
                # end_value = self.visit(range_args[1])
                end = self.get_constant_int_value(range_args[1])
                step = 1
            elif len(range_args) == 3:
                # start_value = self.visit(range_args[0])
                start = self.get_constant_int_value(range_args[0])
                # end_value = self.visit(range_args[1])
                end = self.get_constant_int_value(range_args[1])
                # step_value = self.visit(range_args[2])
                step = self.get_constant_int_value(range_args[2])
            else:
                raise NotImplementedError("range() only supports 1 to 3 parameters")
            assigned_vars = set()
            def collect_assigned_vars(stmts):
                for stmt in stmts:
                    if isinstance(stmt, (ast.Assign, ast.AugAssign)):
                        target = stmt.targets[0] if isinstance(stmt, ast.Assign) else stmt.target
                        if isinstance(target, ast.Name):
                            assigned_vars.add(target.id)
                    elif isinstance(stmt, ast.For):
                        collect_assigned_vars(stmt.body)
            collect_assigned_vars(node.body)
            loop_carried_vars = []
            init_vals = []
            for var_name in assigned_vars:
                if var_name in self.symbol_table:
                    init_val = self.symbol_table[var_name]
                    loop_carried_vars.append(var_name)
                    init_vals.append(init_val)
            loop = affine.AffineForOp(start, end, step, iter_args=init_vals)
            loop_body = loop.body
        self.block_stack.append(loop_body)
        with ir.InsertionPoint(loop_body):
            loop_var = loop.induction_variable
            self.symbol_table[node.target.id] = loop_var
            for idx, var_name in enumerate(loop_carried_vars):
                self.symbol_table[var_name] = loop_body.arguments[idx + 1]
            for stmt in node.body:
                self.visit(stmt)
            yield_vals = [self.symbol_table[var_name] for var_name in loop_carried_vars]
            affine.AffineYieldOp(yield_vals)
        self.block_stack.pop()
        for idx, var_name in enumerate(loop_carried_vars):
            self.symbol_table[var_name] = loop.results[idx]

    def visit_Assign(self, node):
        value = self.visit(node.value)
        target = node.targets[0]
        if isinstance(target, ast.Name):
            target_id = target.id
            self.symbol_table[target_id] = value
            self.variable_versions[target_id] = self.variable_versions.get(target_id, 0) + 1
        else:
            if isinstance(target, ast.Subscript):
                with ir.InsertionPoint(self.block_stack[-1]):
                    self.store_to_subscript(target, value)
            else:
                raise NotImplementedError(f"Unsupported assignment target: {type(target)}")


    def visit_AugAssign(self, node):
        target = node.target
        value = self.visit(node.value)
        if isinstance(target, ast.Name):
            var_name = target.id
            current_value = self.symbol_table[var_name]
            if isinstance(current_value.type, ir.F32Type):
                value = self.cast_to_f64(value)
                with ir.InsertionPoint(self.block_stack[-1]):
                    if isinstance(node.op, ast.Add):
                        op = arith.AddFOp(current_value, value).result
                    elif isinstance(node.op, ast.Sub):
                        op = arith.SubFOp(current_value, value).result
                    elif isinstance(node.op, ast.Mult):
                        op = arith.MulFOp(current_value, value).result
                    elif isinstance(node.op, ast.Div):
                        op = arith.DivFOp(current_value, value).result
                    elif isinstance(node.op, ast.Mod):
                        op = arith.RemFOp(current_value, value).result
                    else:
                        raise NotImplementedError(f"Unsupported operator: {type(node.op)}")
            elif isinstance(current_value.type, (ir.IntegerType)):
                value = self.cast_to_int64(value)
                with ir.InsertionPoint(self.block_stack[-1]):
                    if isinstance(node.op, ast.Add):
                        op = arith.AddIOp(current_value, value).result
                    elif isinstance(node.op, ast.Sub):
                        op = arith.SubIOp(current_value, value).result
                    elif isinstance(node.op, ast.Mult):
                        op = arith.MulIOp(current_value, value).result
                    elif isinstance(node.op, ast.Div):
                        op = arith.DivSIOp(current_value, value).result
                    elif isinstance(node.op, ast.Mod):
                        op = arith.RemSIOp(current_value, value).result
                    else:
                        raise NotImplementedError(f"Unsupported operator: {type(node.op)}")
            self.symbol_table[var_name] = op
        elif isinstance(target, ast.Subscript):
            var_name = self.get_subscript_variable_name(target)
            current_value = self.visit(target)
            with ir.InsertionPoint(self.block_stack[-1]):
                if isinstance(current_value.type, ir.F32Type):
                    value = self.cast_to_f64(value)
                    if isinstance(node.op, ast.Add):
                        result = arith.AddFOp(current_value, value).result
                    elif isinstance(node.op, ast.Sub):
                        result = arith.SubFOp(current_value, value).result
                    elif isinstance(node.op, ast.Mult):
                        result = arith.MulFOp(current_value, value).result
                    elif isinstance(node.op, ast.Div):
                        result = arith.DivFOp(current_value, value).result
                    elif isinstance(node.op, ast.Mod):
                        result = arith.RemFOp(current_value, value).result
                    else:
                        raise NotImplementedError(f"Unsupported compound assignment operation: {type(node.op)}")
                elif isinstance(current_value.type, (ir.IntegerType)):
                    value = self.cast_to_int64(value)
                    if isinstance(node.op, ast.Add):
                        result = arith.AddIOp(current_value, value).result
                    elif isinstance(node.op, ast.Sub):
                        result = arith.SubIOp(current_value, value).result
                    elif isinstance(node.op, ast.Mult):
                        result = arith.MulIOp(current_value, value).result
                    elif isinstance(node.op, ast.Div):
                        result = arith.DivSIOp(current_value, value).result
                    elif isinstance(node.op, ast.Mod):
                        result = arith.RemSIOp(current_value, value).result
                    else:
                        raise NotImplementedError(f"Unsupported compound assignment operation: {type(node.op)}")
            self.store_to_subscript(target, result)
        else:
            raise NotImplementedError(f"Unsupported assignment target: {type(target)}")
        self.variable_versions[var_name] = self.variable_versions.get(var_name, 0) + 1

    def visit_Constant(self, node):
        value = node.value
        with ir.InsertionPoint(self.block_stack[-1]):
            if isinstance(value, int):
                # int_type = ir.IntegerType.get_signless(32)
                # const_op = arith.ConstantOp(int_type, value)
                f64 = ir.F32Type.get()
                const_op = arith.ConstantOp(f64, float(value))
            elif isinstance(value, float):
                f64 = ir.F32Type.get()
                const_op = arith.ConstantOp(f64, value)
            else:
                raise NotImplementedError(f"Unsupported constant type: {type(value)}")
        return const_op.result
