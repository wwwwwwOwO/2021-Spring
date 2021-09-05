import math
import numpy as np
from numpy import *


def Reform(a,  b):  # 列主元
    A_b = np.column_stack((a, b))  # 将两个矩阵a，b按列合并
    row = A_b.shape[0]  # 初始化row为矩阵[a b]的阶数
    for i in range(0, row):
        if i < row:
            maxRow = np.argmax(abs(A_b[i:, i]))     # 找到最大元素对应的行。最后一个元素不用找最大值；只找对角线下方的主元
        else:
            maxRow = 0
        b1 = maxRow + i                      # 下面几行是主元和对角线所在行交换
        temp = np.copy(A_b[b1, :])
        A_b[b1, :] = A_b[i, :]
        A_b[i, :] = temp

    return A_b


def GaussS(a, b,  x):
    A_b = Reform(a, b)     # 预处理，将每一列最大值移到对角线，这里的p最后一行是b
    row = A_b.shape[0]     # 获取行数
    a0 = A_b[:, 0:row]      # 系数矩阵
    b0 = A_b[:, row]        # b矩阵
    iterations = 0
    err = 100.        # 初始化

    print("")
    print("Gauss-Seidel method:")

    while(err > 1.e-7 and iterations < 1000):  # 控制迭代次数
        i = 0
        while(i < x.size):  # 控制循环次数
            if a0[i, i] == 0:
                print('a[i, i]  = 0,  i = ', i)
            x[i] = (b0[i] - np.dot(a0[i, :], x) + a0[i, i] * x[i]) / a0[i, i]
            i = i+1
        iterations = iterations+1
        err = Norm(a0, b0,  x)  # 计算ax-b二范数
# print(iterations, x)
    return x, iterations


def Relax(a, b,  x, omega):
    A_b = Reform(a, b)     # 预处理，将每一列最大值移到对角线，这里的p最后一行是b
    row = A_b.shape[0]     # 获取行数
    a0 = A_b[:, 0:row]      # 系数矩阵
    b0 = A_b[:, row]        #
    err = 100.
    iterations = 0

    # print("")
    # print("SOR method:")

    while(err > 1.e-7 and iterations < 1000):
        i = 0
        while(i < x.size):  # 控制循环次数
            if a0[i, i] == 0:
                print('a[i, i] = 0, i = ',  i)
            x[i] = (1 - omega)*x[i] - omega * (np.dot(a0[i, :], x) - b0[i] - a0[i, i] * x[i])/a0[i, i]
            i = i + 1
        iterations = iterations+1
        err = Norm(a0, b0,  x)  # 计算ax-b二范数
# print(iterations,  x)
    return x,  iterations


def Norm(a, b,  x):  # 计算范数
    axb = np.dot(a, x)-b
    normaxb = np.linalg.norm(axb,  ord=2)   # 二范数
    return normaxb

# 主程序


n = 7  # 设置阶数为7
# a = np.array([
#  [9, 3, 4, 0, 0, 0, 0],
#  [1, 5, 1, 0, 0, 0, 0],
#  [0, 3, 8, 2, 0, 0, 0],
#  [0, 0, 1, 7, 3, 0, 0],
#  [0, 0, 0, 2, 6, 3, 0],
#  [0, 0, 0, 0, 1, 3, 1],
#  [0, 0, 0, 0, 0, 1, 4]
# ])
# print(sum(abs(a[0, :])))
a = np.random.randint(0, 10, (n, n))  # 随机生成矩阵

# 生成严格对角占优矩阵，若不需要该条件则注释掉以下部分代码
i = 0
while(i < n):  # 控制循环次数
    while(abs(a[i, i]) < sum(abs(a[i, :]))-abs(a[i, i])):  # 对每一行对角线上的元素，使它大于所在行其他元素的绝对值之和
        a[i, i] = a[i, i] + 1
    i = i + 1

answer = np.random.randint(-10, 10, (n, 1))
b = np.dot(a, answer)

x = np.zeros(n, dtype=float)
print('A=')
print(a)
print('Answer=')
print(answer)
print('b=')
print(b)

# x1, iterations = GaussS(np.copy(a), np.copy(b), np.copy(x))
# print('GS method iterations = ', iterations)
# print('x = ')
# print(x1)

omega = 1.2
x2, iterations = Relax(np.copy(a), np.copy(b), np.copy(x),  omega)
print('SOR method iterations = ', iterations)
print('w = ', omega)
print('x = ')
print(x2)

# print('SOR method iterations = ')
# while (omega <= 4):
#     x2, iterations = Relax(np.copy(a), np.copy(b), np.copy(x),  omega)
#     print('w = ', omega, ' iterations = ', iterations)
#     print('x = ')
#     print(x2)
#     print("")
#     omega = omega + 0.5
