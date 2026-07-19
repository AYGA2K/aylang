#pragma once

#include "parser/expression.h"
#include <functional>
#include <memory>

using ExprPtr = std::unique_ptr<Expression>;

using UnaryParseFn = std::function<ExprPtr()>;
using BinaryParseFn = std::function<ExprPtr(ExprPtr)>;
