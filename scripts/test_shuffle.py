import subprocess
import re
import random
from itertools import permutations
import random
from scipy.stats import chi2


# 測試 shuffle 次數
test_count = 1000000

input_commands = ["new\n", "it 1\n", "it 2\n", "it 3\n", "it 4\n"]
input_commands.extend(["shuffle\n"] * test_count)
input_commands.append("free\nquit\n")
input = "".join(input_commands)


# 取得 stdout 的 shuffle 結果
command='./qtest -v 3'
clist = command.split()
completedProcess = subprocess.run(clist, capture_output=True, text=True, input=input)
s = completedProcess.stdout
startIdx = s.find("l = [1 2 3 4]") 
endIdx = s.find("l = NULL")
s = s[startIdx + 14 : endIdx]
Regex = re.compile(r'\d \d \d \d')
result = Regex.findall(s)

def permute(nums):
    nums=list(permutations(nums,len(nums)))
    return nums

def chiSquared(observation, expectation):
    return ((observation - expectation) ** 2) / expectation 

# shuffle 的所有結果   
nums = []
for i in result:
    nums.append(i.split())

# 找出全部的排序可能
counterSet = {}
shuffle_array = ['1', '2', '3', '4']
s = permute(shuffle_array)

# 初始化 counterSet
for i in range(len(s)):
    w = ''.join(s[i])
    counterSet[w] = 0

# 計算每一種 shuffle 結果的數量
for num in nums:
    permutation = ''.join(num)
    counterSet[permutation] += 1
        
# 計算 chiSquare sum
expectation = test_count // len(s)
c = counterSet.values()
chiSquaredSum = 0
for i in c:
    chiSquaredSum += chiSquared(i, expectation)

# 4 個數字共有 4! = 24 種可能，自由度 = 23
df = 23

p_value = chi2.sf(chiSquaredSum, df)

print("Expectation: ", expectation)
print("Observation: ", counterSet)
print("chi square sum: ", chiSquaredSum)
print("p value:", p_value)

import matplotlib.pyplot as plt
import matplotlib
import numpy as np
permutations = counterSet.keys()
counts = counterSet.values()


matplotlib.use("TkAgg")

x = np.arange(len(counts))
plt.bar(x, counts)
plt.xticks(x, permutations)
plt.xlabel('permutations')
plt.ylabel('counts')
plt.title('Shuffle result')
plt.show()

