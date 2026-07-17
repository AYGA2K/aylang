#pragma once

#include "parser/expression.h"
#include <functional>
#include <memory>

using ExprPtr = std::unique_ptr<Expression>;

using PrefixParseFn = std::function<ExprPtr()>;
using InfixParseFn = std::function<ExprPtr(ExprPtr)>;
