#ifndef POLYNOMIAL_DERIVATIVE_CORE_H
#define POLYNOMIAL_DERIVATIVE_CORE_H

// 公共接口，第 1 版。使用 C++17。
// 本文件只定义公共类型、使用约定和函数声明；具体算法放在各 .cpp 中。
// 修改函数名、参数、返回值或数据结构前，请先通知使用它们的队友。

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// ---------- 统一错误：所有模块使用，主程序负责捕获并显示 ----------
// position 是原输入从 0 开始的字符下标，显示时加 1。
// 无法对应原输入的位置时使用 noPosition；此时界面不要显示字符编号。
struct CalcError : public std::runtime_error {
    static constexpr std::size_t noPosition = std::string::npos;
    std::size_t position;

    explicit CalcError(const std::string& message,
                       std::size_t inputPosition = noPosition)
        : std::runtime_error(message), position(inputPosition) {}
};

// ---------- 分数：岗位 D 实现相关函数 ----------
// 所有对外提供的分数：分母 > 0，已经约分，零为 0/1。
// 除默认值外，优先通过 makeRational 创建分数，不要随意修改字段。
// long long 有范围限制；不能把任意大整数视为本项目已支持的能力。
struct Rational {
    long long numerator = 0;
    long long denominator = 1;
};

Rational makeRational(long long numerator, long long denominator = 1);
Rational addRational(const Rational& a, const Rational& b);
Rational subtractRational(const Rational& a, const Rational& b);
Rational multiplyRational(const Rational& a, const Rational& b);
Rational divideRational(const Rational& a, const Rational& b);
// 分母为 0 或除以零时抛出 CalcError；分数运算不使用浮点数。

// ---------- 表达式树：岗位 A 建树，岗位 B 求导 ----------
enum class NodeKind {
    Number, Variable,
    Add, Subtract, Multiply, Divide, Power, Negate
};

struct ExprNode {
    NodeKind kind = NodeKind::Number;
    Rational value;  // 仅 Number 使用；数字的正负优先由 Negate 表达。
    std::size_t position = CalcError::noPosition;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;
};

using ExprTree = std::unique_ptr<ExprNode>;

// Number / Variable：没有子节点。Variable 代表唯一变量 x。
// Negate：仅 left 非空，表示 -(left)。
// Add / Subtract / Multiply / Divide / Power：left、right 均非空。
// Power：right 为整数 Number 节点，数值 >= 0；left 可为括号表达式。
// Divide：right 为常数 Number 节点，可由一元负号包裹；必须非零。
// 本版不支持 x/(x+1)、x/(1+2)、x^x、x^-1、sin(x)。
// 将来若放宽“除号右侧常数”的语法，先共同确认题目解释并更新约定。

// 岗位 A：输入 -> 新建的非空表达式树；失败时抛出 CalcError。
// 支持题目基础语法，另支持括号和任意位置的一元负号（如 x*-x）。
// 常数紧跟 x 时可省略乘号（6x）；其他位置不额外放宽（2(x+1) 不支持）。
// 不要求支持空格、小数和其他变量。
// 优先级：括号 > 乘方 > 一元正负号 > 乘除 > 加减。
// 乘除、加减分别左结合；-x^2 表示 -(x^2)，--x 表示 -(-x)。
// 解析时检查指数与除数约束，不把不支持的写法静默解释成其他含义。
ExprTree parseExpression(const std::string& input);

// 岗位 B：原式树 -> 新建的非空导数树；不修改原树。
// 可保留 0*x+3*1 等未化简形式；由 toPolynomial 负责化简。
// 子树需要重复出现时创建独立副本，不把同一节点交给两个 unique_ptr。
// 指数为 0 的求导单独处理，避免构造负指数节点。
ExprTree differentiate(const ExprNode& expression);

// ---------- 规范多项式：岗位 C 实现 ----------
struct Term {
    Rational coefficient;
    int exponent = 0;
};

using Polynomial = std::vector<Term>;

// “规范”统一指：指数非负且严格降序；无重复指数；无零系数项；
// 系数是规范分数；空 vector 表示零多项式。
// 下列多项式函数输入应符合该约定，输出也必须符合；不修改输入。

// 原式树、导数树均可转化；递归处理允许的全部运算。
// 除以常数即各项系数除以该常数；不要求支持多项式除法。
// 本项目将包括 0^0 在内的整数 0 次幂按多项式约定视为 1。
Polynomial toPolynomial(const ExprNode& expression);
Polynomial add(const Polynomial& a, const Polynomial& b);
Polynomial subtract(const Polynomial& a, const Polynomial& b);
Polynomial multiply(const Polynomial& a, const Polynomial& b);

// 降幂输出，无空格；空表输出 "0"。
// 非常数项的系数 +/-1 省略数字 1；常数 +/-1 正常显示。
// 指数 1 省略 ^1，指数 0 只显示系数；首项为正不写 +。
// 分数用最简假分数，例如 2/3x、-1/2x^2；不输出小数或带分数。
std::string formatPolynomial(const Polynomial& polynomial);

// 后续提高要求预留：代入数值求值（岗位 C，决定开展时再实现）。
// 允许使用 double；函数声明存在不代表当前已实现。
double evaluate(const Polynomial& polynomial, double x);

// ---------- 主程序与整合：岗位 D 编写 main，岗位 B 协调整合 ----------
// 主流程：parseExpression -> differentiate -> toPolynomial -> formatPolynomial。
// 用 const ExprNode& 的函数接收树的根节点时，传 *tree，先保证 tree 非空。
// main 捕获 const CalcError&；通过 what() 取原因，通过 position 取位置。
// 各算法函数不自行打印、不读取键盘、不直接结束程序。
// 每个 .cpp 都 #include "core.h"；全项目仅有一个正式程序的 main。
// 测试若有独立 main，应构建为独立测试程序，避免和正式 main 一起链接。
// 输入扫描及树求导避免反复扫描/复制完整表达式；复杂度分别分析。
// 求导树及展开结果可能增长，不把整个流程笼统宣称为线性复杂度。

#endif  // POLYNOMIAL_DERIVATIVE_CORE_H
