# 這次的更新
## Part 1
隨機生成小dataset (100K)，測試不同分類方法:

   kmeans k=3
  
   kmeans k=3 每群移除5%
  
   平分三等份
  
   不分 全部一起放 
  
  
## Part 2
使用先前生成好的100M 80%、40%、30%，測試不同分類方法:
   kmeans k=3 每群移除5%

   kmeans k=3

   平分三等份 (90-10都做了)

   不分 全部一起放 

這邊的kmeans都是c++實現python kmeans邏輯，會有分不準的情形，但performance比python好??

數據差很多的話可能是有沒有取平均(/100)的差別
